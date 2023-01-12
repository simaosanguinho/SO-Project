#include "logging.h"
#include "utils.h"
#include "operations.h"
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

#define BUFFER_SIZE 1024

int max_sessions = 0;
int curr_sessions = 0;
int register_pipe; /* pipe opened */

typedef struct box {
	char box_name[32];
	int publisher;
	int *subscribers;
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

/*             */
/*     BOX     */
/*             */

// insert box in the beginning of the list
void insert_box(char box_name[32]) {
	Box* new_box = (Box*)malloc(sizeof(Box));
	int subscribers[0];
	new_box->subscribers = subscribers;
	strcpy(new_box->box_name, box_name);
	new_box->publisher = -1;
	new_box->next = box_list;
	pthread_cond_init(&new_box->condition, NULL);
	box_list = new_box;
}

void delete_box(char box_name[32]) {
	Box* temp = box_list;
	Box* prev = NULL;
	if (temp != NULL && strcmp(temp->box_name, box_name) == 0) {
		box_list = temp->next;
		free(temp);
		return;
	}
	while (temp != NULL && strcmp(temp->box_name, box_name) != 0) {
        prev = temp;
        temp = temp->next;
    }
    if (temp == NULL) return;
    prev->next = temp->next;
    free(temp);
}

/*
void print_list() {
	Box* temp = box_list;
    while (temp != NULL) {
        // PROCESSAR DADOS
        temp = temp->next;
    }
}
*/

Box* search_boxes(char box_name[32]) {
	Box* temp = box_list;
	if (temp == NULL) {
		return temp;
	}
    while (temp != NULL && strcmp(temp->box_name, box_name) != 0) {
        temp = temp->next;
    }
	if (strcmp(temp->box_name, box_name) == 0) {
		return temp;
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

/* Check if box exists, doesn't have other publisher
	and doesn't exceed session limits */
int register_pub(char *name_pipe, char *box_name) {

	Box* box = search_boxes(box_name);

    int pub_pipe = open(name_pipe, O_RDONLY);
    if (pub_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	if (box == NULL) {
		close(pub_pipe);
		return -1;
	// box already has a publisher
	} else if (box->publisher != -1) {
		close(pub_pipe);
		return -1;
	} else {
		box->publisher = pub_pipe;
	}

	return pub_pipe;
}


void read_publisher(int pub_pipe) {
	while (true) {
		char buffer[BUFFER_SIZE];
		memset(buffer, '\0', BUFFER_SIZE);
		ssize_t ret = read(pub_pipe, buffer, BUFFER_SIZE);
		if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		} else if (ret==0) {
			curr_sessions--;
			break;
		} else {
			buffer[ret] = 0;
			fprintf(stderr, "[PUBLISHER] %s\n", buffer);
			/* ENVIAR MENSAGEM NO LOCAL APROPRIADO */
		}
	}
}


/*                    */
/*     SUBSCRIBER     */
/*                    */

int register_sub(char *name_pipe, char *box_name) {
	(void) box_name;
	// Open pipe for write
    int pub_pipe = open(name_pipe, O_WRONLY);
    if (pub_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	// VERIFICAR SE PODE HAVER CONEXÃO
	if (curr_sessions < max_sessions) {
		curr_sessions++;
		return pub_pipe;
	}
	return -1;
}

void write_subscriber(int pub_pipe) {
	while (true) {
		char buffer[BUFFER_SIZE];
		memset(buffer, '\0', BUFFER_SIZE);

		ssize_t ret = write(pub_pipe, buffer, BUFFER_SIZE);
		if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		} else if (ret==0) {
			curr_sessions--;
			break;
		} else {
			buffer[ret] = 0;
			fprintf(stderr, "[PUBLISHER] %s\n", buffer);
			/* ENVIAR MENSAGEM NO LOCAL APROPRIADO */
		}
	}
}

/*                 */
/*     MANAGER     */
/*                 */

int register_new_box(char *name_pipe, char *box_name) {
	// Open pipe for write
    int pub_pipe = open(name_pipe, O_WRONLY);
    if (pub_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	// VERIFICAR SE PODE HAVER CONEXÃO
	if (curr_sessions > max_sessions) {
		return -1;
	}

	/* Caixa já existe */
	if (search_boxes(box_name) != NULL) {
		// RETORNAR ERRO
	}

	int fhandler = tfs_open(box_name, TFS_O_CREAT);
	
	if (fhandler == -1) {
		// RETORNAR ERRO
	}

	insert_box(box_name);

	tfs_close(fhandler);

	return -1;
}

// convert a string of lenght 1 to character
char string_to_char(char* str){
	char c = str[0];
	return c;
}



void process_serialization(char *message) {
	char *args = strtok(message, "|");
	char code = string_to_char(args);

	/* Register Publisher */
	if (REQUEST_PUB_REGISTER == code) {
		char name_pipe[256];
		char box_name[32];
		strcpy(name_pipe, strtok(NULL, "|"));
		strcpy(box_name, strtok(NULL, "|"));
		int pub_pipe = register_pub(name_pipe, box_name);
		if (pub_pipe != -1) {
			read_publisher(pub_pipe);
			close(pub_pipe);
		}
	/* Register Subscriber */
	} else if (REQUEST_SUB_REGISTER == code) {
		char name_pipe[256];
		char box_name[32];
		strcpy(name_pipe, strtok(NULL, "|"));
		strcpy(box_name, strtok(NULL, "|"));
		int sub_pipe = register_sub(name_pipe, box_name);
		if (sub_pipe != -1) {
			//write_subscriber(pub_pipe);
		}
		close(sub_pipe);

	/* Create Box */
	} else if (REQUEST_BOX_CREATE == code) {
		char name_pipe[256];
		char box_name[32];
		strcpy(name_pipe, strtok(NULL, "|"));
		strcpy(box_name, strtok(NULL, "|"));
		int box_pipe = register_new_box(name_pipe, box_name);
		close(box_pipe);
	}
}


int main(int argc, char **argv) {
    // initialise file system
    assert(tfs_init(NULL) != -1);

	/* Verify and store arguments given */
    verify_arguments(argc);
    char* register_pipe_name = argv[1];
    max_sessions = atoi(argv[2]);

    register_pipe = mbroker_init(register_pipe_name);
    
    while (true) {
        char buffer[BUFFER_SIZE];
		memset(buffer, '\0', BUFFER_SIZE);
        ssize_t ret = read(register_pipe, buffer, BUFFER_SIZE);
        if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (ret != 0) {
			buffer[ret] = 0;
			process_serialization(buffer);
		}
    }

    return -1;
}
