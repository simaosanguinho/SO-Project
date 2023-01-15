#include "logging.h"
#include "utils.h"
#include "operations.h"
#include "producer-consumer.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pthread.h>

int max_sessions = 0;
int register_pipe; /* pipe opened */
pc_queue_t* queue;
pthread_t* threads;


typedef struct box {
	char box_name[32];
	uint64_t publisher;
	uint64_t subscribers;
	uint64_t box_size;
	pthread_mutex_t mutex;
	pthread_cond_t condition;
	struct box* next;
} Box;


Box* box_list = NULL;


void print_usage(){
    fprintf(stderr, "usage: mbroker <pipename> <maxsessions>\n");
}

/* verify arguments */
int verify_arguments(int argc){
    if(argc != 3){
        print_usage();
        exit(EXIT_FAILURE);
    }
    return 0;
}

/* returns filename with a '/' in the beginning */
void get_box_filename(char box_name[BOX_SIZE], char *filename) {
	sprintf(filename, "/%s", box_name);
}


/*             */
/*     BOX     */
/*             */

// insert box in the beginning of the list
void insert_box(char box_name[32]) {
	Box* new_box = (Box*)malloc(sizeof(Box));
	strcpy(new_box->box_name, box_name);
	new_box->subscribers = 0;
	new_box->publisher = 0;
	new_box->box_size = 0;
	new_box->next = box_list;
	pthread_cond_init(&new_box->condition, NULL);
	pthread_mutex_init(&new_box->mutex, NULL);
	box_list = new_box;
}

int delete_box(char *box_name) {
	Box* temp = box_list;
	Box* prev = NULL;
	if (temp != NULL && strcmp(temp->box_name, box_name) == 0) {
		box_list = temp->next;
		//pthread_cond_broadcast(&temp->condition);
		free(temp);
		temp = NULL;
		return 0;
	}
	while (temp != NULL && strcmp(temp->box_name, box_name) != 0) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return -1;
    prev->next = temp->next;
    free(temp);
	return 0;
}


Box* search_boxes(char *box_name) {
	Box* temp = box_list;

    while (temp != NULL) {
		if (strcmp(temp->box_name, box_name) == 0) {
			return temp;
		}
		temp = temp->next;
    }

	return NULL;
}

/* returns file desciptor of pipe that represents mbroker*/
int mbroker_init(char* register_pipe_name){
    
	// Remove pipe if it does not exist
    if (unlink(register_pipe_name) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", register_pipe_name,
        strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Create register pipe
    if (mkfifo(register_pipe_name, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Open register pipe for reading
    int register_pipee = open(register_pipe_name, O_RDONLY);
    if (register_pipee == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return register_pipee;
}


/*                   */
/*     PUBLISHER     */
/*                   */

/*  */
void read_publisher(int pub_pipe, Box* box) {
	while (true) {
		MessageRequest request;

		ssize_t ret = read(pub_pipe, &request, sizeof(request));
		if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		} else if (ret==0 || box == NULL) {
			/* Publisher closed session or box doesn't exist anymore */
			break;
		} else {
			pthread_mutex_lock(&box->mutex);
			char filename[BOX_SIZE+1];
			/* Write message into the box in tfs */
			get_box_filename(box->box_name, filename);

			int fbox = tfs_open(filename, TFS_O_APPEND); /* APPEND flag to force offset to write in the end */
			ssize_t written = tfs_write(fbox, request.message, strlen(request.message)+1); /* strlen+1 forces \0 character to be written */
			if (written == -1) {
				/* Error writing */
				pthread_mutex_unlock(&box->mutex);
				continue;
			} else if (written != strlen(request.message)+1) {
				/* File exceed maximum capacity */
				box->box_size += (uint64_t) written;
				pthread_mutex_unlock(&box->mutex);
				tfs_close(fbox);
				break;
			}
			/* Broadcast a signal to all subscribers to receive a message */
			pthread_mutex_unlock(&box->mutex);
			pthread_cond_broadcast(&box->condition);

			// increment the box size with the new message size
			box->box_size += (uint64_t) written;
			
			tfs_close(fbox);
		}
	}
	if (box != NULL) {
		box->publisher=0;
	}
	close(pub_pipe); /* Close session */
}


/* Check if box exists, doesn't have other publisher
	and doesn't exceed session limits */
void register_pub(char *name_pipe, char *box_name) {

	Box* box = search_boxes(box_name);

    int pub_pipe = open(name_pipe, O_RDONLY);
    if (pub_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	/* Verify if box exists */
	if (box == NULL) {
		close(pub_pipe);

		return;
	/* Verify if box alread has a publisher */
	} else if (box->publisher != 0) {
		close(pub_pipe);
		return;
	} else {
		box->publisher = 1;
	}

	read_publisher(pub_pipe, box);
}



/*                    */
/*     SUBSCRIBER     */
/*                    */

int send_message_subscriber(int sub_pipe, char message[MESSAGE_SIZE]) {
	MessageRequest request;
	request.code = SUB_READ_MSG;
	memset(request.message, '\0', sizeof(request.message));
	memcpy(request.message, message, strlen(message));
	
	ssize_t written = write(sub_pipe, &request, sizeof(request));
	if (written == -1) {
		if (errno == EPIPE) {
			return -1; /* sub pipe session was closed and unliked */
		}
		fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	} else if (written==0) {
		return -1; /* subscriber session closed */
	}
	return 0;
}

int read_past_messages(int sub_pipe, Box* box) {
	if (box == NULL) {
		return -1;
	}
	char filename[BOX_SIZE+1];
	get_box_filename(box->box_name, filename);
	int fbox = tfs_open(filename, 0); /* Open to read, with offset in 0 */
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE);
	ssize_t bytes_read;
	pthread_mutex_lock(&box->mutex);
	while ((bytes_read = tfs_read(fbox, buffer, BUFFER_SIZE)) > 0) {

		char message[MESSAGE_SIZE];
		memset(message, '\0', MESSAGE_SIZE);
		int index = 0;

		for (int i=0; i<bytes_read; i++) { /* find null char */
			if (buffer[i] == '\0') {
				//usleep(1000); /* Prevenir race com o read do subscriber - sleep 0.1 ms */
				if (send_message_subscriber(sub_pipe, message) == -1) { /* send message to subscriber */
						box->subscribers--;
						tfs_close(fbox);
						close(sub_pipe); /* close session */
						pthread_mutex_unlock(&box->mutex);
						return -1;
				}
				memset(message, '\0', MESSAGE_SIZE);
				index = 0;
			} else {
				message[index] = buffer[i];
				index++;
			}
		}
	}
	pthread_mutex_unlock(&box->mutex);
	tfs_close(fbox);
	return 0;
}


void write_subscriber(int sub_pipe, Box* box) {
	if (read_past_messages(sub_pipe, box) == -1) {
		return; /* Error reading old messages, close session */
	}

	char filename[BOX_SIZE+1];
	get_box_filename(box->box_name, filename);
	int fbox = tfs_open(filename, TFS_O_APPEND); /* Force to set offset in last position */
	pthread_mutex_lock(&box->mutex);
	while (true) { /* Read new messages */
		if (box == NULL) { /* Box doesn't exist anymore */
			break; /* Close session */
		}
		pthread_cond_wait(&box->condition, &box->mutex);
		char buffer[BUFFER_SIZE];
		memset(buffer, '\0', BUFFER_SIZE);

		ssize_t written = tfs_read(fbox, buffer, MESSAGE_SIZE); /* read last message */
		if (written == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			pthread_mutex_unlock(&box->mutex);
			exit(EXIT_FAILURE);
		} else if (written==0) {
			box->subscribers--;
			break; /* session closed */
		} else {
			buffer[written] = 0;
			if (send_message_subscriber(sub_pipe, buffer) == -1) {
				pthread_mutex_unlock(&box->mutex);
				box->subscribers--;
				return; // close session (error sending message)
			}
		}
	}
	pthread_mutex_unlock(&box->mutex);
	tfs_close(fbox);
	close(sub_pipe);
}


void register_sub(char *name_pipe, char *box_name) {

	Box* box = search_boxes(box_name); /* Get the box */

    int sub_pipe = open(name_pipe, O_WRONLY);
    if (sub_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	/* Verify if box exists */
	if (box == NULL) {
		close(sub_pipe);
		return;
	} else {
		// adicionar subscriber à box
		box->subscribers++;
	}

	write_subscriber(sub_pipe, box);
	box->subscribers--;
}

/*                 */
/*     MANAGER     */
/*                 */


void register_new_box(char name_pipe[PIPE_SIZE], char box_name[BOX_SIZE]) {
	// Open pipe for write
    int man_pipe = open(name_pipe, O_WRONLY);
    if (man_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	MessageRequest response;
	response.code = ANSWER_BOX_CREATE;
	response.return_code = 0;
	memset(response.message, '\0', sizeof(response.message));

	// box already exists
	if (search_boxes(box_name) != NULL) {
		response.return_code = -1;
		strcpy(response.message, "Box already exists.");
	} else {
		char filename[BOX_SIZE+1];
		get_box_filename(box_name, filename);
		int fhandler = tfs_open(filename, TFS_O_CREAT);
		// could not open a tfs_file 
		if (fhandler == -1) {
			response.return_code = -1;
			strcpy(response.message, "Error creating new file on tfs.");
		} else {
			insert_box(box_name);
		}
		tfs_close(fhandler);
	}
	
	ssize_t written = write(man_pipe, &response, sizeof(response));
	if (written < 0) {
		exit(EXIT_FAILURE);
	}

	close(man_pipe);
}



void remove_box(char *name_pipe, char *box_name){
	MessageRequest response;
	response.code = ANSWER_BOX_REMOVE;
	response.return_code = 0;
	memset(response.message, '\0', sizeof(response.message));

	// Open pipe for write
    int man_pipe = open(name_pipe, O_WRONLY);
    if (man_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	// the subscribers and publishers' pipes close automatically
	// so these stop running 
	if (delete_box(box_name) == -1){
		response.return_code = -1;
		strcpy(response.message, "deleting box - box does not exist.");
	}

	// write message in pipe
	ssize_t written = write(man_pipe, &response, sizeof(response));
	if (written < 0) {
		exit(EXIT_FAILURE);
	}
	char filename[BOX_SIZE+1];
	get_box_filename(box_name, filename);
	tfs_unlink(box_name);
	close(man_pipe);
}


void send_box_list(char* name_pipe) {
	MessageRequest response;
	response.code = ANSWER_BOX_LIST;

	int man_pipe = open(name_pipe, O_WRONLY);
    if (man_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	Box* temp = box_list;
	if (temp == NULL) { /* no box found */
		memset(response.box_name, '\0', sizeof(response.box_name));
		response.last = 1;
		if (write(man_pipe, &response, sizeof(response))<0) exit(EXIT_FAILURE);
	}
    while (temp != NULL) {
		memset(response.box_name, '\0', sizeof(response.box_name));
		if (temp->next == NULL) response.last = 1;
		memcpy(response.box_name, temp->box_name, strlen(temp->box_name));
		response.box_size = temp->box_size;
		response.n_publishers = temp->publisher;
		response.n_subscribers = temp->subscribers;

		//usleep(1000); //prevent races
		if (write(man_pipe, &response, sizeof(response)) == -1) 
			exit(EXIT_FAILURE);
        temp = temp->next;
    }
	
	close(man_pipe);
}


void process_serialization(MessageRequest *request) {
	uint8_t code = request->code;

	/* Register Publisher */
	if (REQUEST_PUB_REGISTER == code) {
		char name_pipe[PIPE_SIZE];
		char box_name[BOX_SIZE];
		memcpy(name_pipe, request->pipe_name, sizeof(request->pipe_name));
		memcpy(box_name, request->box_name, sizeof(request->box_name));
		register_pub(name_pipe, box_name);

	/* Register Subscriber */
	} else if (REQUEST_SUB_REGISTER == code) {
		char name_pipe[PIPE_SIZE];
		char box_name[BOX_SIZE];
		memcpy(name_pipe, request->pipe_name, sizeof(request->pipe_name));
		memcpy(box_name, request->box_name, sizeof(request->box_name));
		register_sub(name_pipe, box_name);

	/* Create Box */
	} else if (REQUEST_BOX_CREATE == code) {
		char name_pipe[PIPE_SIZE];
		char box_name[BOX_SIZE];
		memcpy(name_pipe, request->pipe_name, sizeof(request->pipe_name));
		memcpy(box_name, request->box_name, sizeof(request->box_name));
		register_new_box(name_pipe, box_name);

	/* Remove Box */
	} else if (REQUEST_BOX_REMOVE == code) {
		char name_pipe[PIPE_SIZE];
		char box_name[BOX_SIZE];
		memcpy(name_pipe, request->pipe_name, sizeof(request->pipe_name));
		memcpy(box_name, request->box_name, sizeof(request->box_name));
		remove_box(name_pipe, box_name);

	/* List Boxes */
	} else if (REQUEST_BOX_LIST == code){
		char name_pipe[PIPE_SIZE];
		memcpy(name_pipe, request->pipe_name, sizeof(request->pipe_name));
		send_box_list(name_pipe);
	}
}



void *session_threads() {
	while (true) {
		MessageRequest *request = (MessageRequest *)pcq_dequeue(queue);
		process_serialization(request);
	}
}


void close_queue() {
	pcq_destroy(queue);
	free(queue);
	free(threads);
}

void destroy_boxes(Box* box) {
	if (box == NULL) {
		return;
	} else {
		Box* temp = box;
		box = box->next;
		free(temp);
		destroy_boxes(box);
	}
}

static void sig_handler(int sig) {
	if (sig == SIGINT) {
		if (signal(SIGINT, sig_handler) == SIG_ERR) {
			exit(EXIT_FAILURE);
		}
	} else if (sig == SIGQUIT) {
		if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
			exit(EXIT_FAILURE);
		}
	}
	close_queue();
	destroy_boxes(box_list);
	exit(EXIT_SUCCESS);
}
    


int main(int argc, char **argv) {
    // initialise file system
    assert(tfs_init(NULL) != -1);

	/* Verify and store arguments given */
    verify_arguments(argc);
    char* register_pipe_name = argv[1];
    max_sessions = atoi(argv[2]);

	/* Producer-Consumer Queue Init */
	threads = (pthread_t*)malloc(sizeof(pthread_t)*(unsigned int)max_sessions);
	size_t queue_size = (size_t) max_sessions*2;
	// allocate memory for the queue
	queue = (pc_queue_t*)malloc(sizeof(pc_queue_t));
	
	if (pcq_create(queue,queue_size) == -1) {
		exit(EXIT_FAILURE);
	}

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		exit(EXIT_FAILURE);
	} else if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
		exit(EXIT_FAILURE);
	} else if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		exit(EXIT_FAILURE);
	}
	
	//for a iniciar as threads
	for(int i=0; i<max_sessions; i++){
		assert(pthread_create(&threads[i], NULL, session_threads, NULL) == 0); 
	}



    register_pipe = mbroker_init(register_pipe_name);

    while (true) {
		MessageRequest request;
        ssize_t ret = read(register_pipe, &request, BUFFER_SIZE);
        if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (ret != 0) {
			if (pcq_enqueue(queue, &request) == -1) {
				exit(EXIT_FAILURE);
			}		}
    }

    return -1;
}
