#include "logging.h"
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
#include "utils.h"
#include "operations.h"



typedef struct box_data {
	char box_name[BOX_SIZE];
	long box_size;
	long n_subscribers;
	long publisher;
	struct box_data* next;
} Box_Data;

Box_Data* box_list = NULL;

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

/* verify arguments */
int verify_arguments(int argc){
    if(argc != 4 && argc != 5){
        print_usage();
        exit(EXIT_FAILURE);
    }
    return 0;
}


void manager_init(char* pipe_name){

    // Remove pipe if it does not exist
    if (unlink(pipe_name) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipe_name,
        strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Create pipe
    if (mkfifo(pipe_name, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

}


int open_register_pipe(char* register_pipe_name){

	// Open register pipe for writing
    int register_pipe = open(register_pipe_name, O_WRONLY);
    if (register_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
	return register_pipe;

}

void close_register_pipe(int register_pipe){
	close(register_pipe);
}

// tries to create or destroy a box 
void create_destroy_box(uint8_t code, char box_name[BOX_SIZE], char pipe_name[PIPE_SIZE], int register_pipe){
	/* Format message request */
	uint8_t message_request[sizeof(uint8_t)+(BOX_SIZE+PIPE_SIZE)*sizeof(char)] = {0};
	memcpy(message_request, &code, sizeof(uint8_t));
	memcpy(message_request+sizeof(uint8_t), pipe_name, strlen(pipe_name));
	memcpy(message_request+sizeof(uint8_t)+PIPE_SIZE*sizeof(char), box_name, strlen(box_name));

	/* Send request */
	if (write(register_pipe, message_request, sizeof(message_request)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
	}
    close_register_pipe(register_pipe);
	int pipe_i = open(pipe_name, O_RDONLY);
    if (pipe_i == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }


	uint8_t message_response[sizeof(uint8_t)+sizeof(int32_t)+sizeof(char)*MESSAGE_SIZE] = {0};
	int32_t return_code;

	ssize_t ret = read(pipe_i, message_response, sizeof(message_response));
	if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
	}
	memcpy(&return_code, message_request+sizeof(uint8_t), sizeof(int32_t));

	// box was successfully created or destroyed
	if (return_code != -1) {
		fprintf(stdout, "OK\n");
	} else {
		char error_message[MESSAGE_SIZE];
		memcpy(error_message, message_response+sizeof(uint8_t)+sizeof(int32_t), MESSAGE_SIZE*sizeof(char));
		fprintf(stdout, "ERROR %s\n", error_message);
	}
	
	close(pipe_i);
}

// insert box in list by alphabetical order
void insert_box(Box_Data *box){

	// insert at the beginning
	if (box_list == NULL || strcmp(box_list->box_name, box->box_name) > 0) {
        box->next = box_list;
        box_list = box;
	}
	else {
        Box_Data* temp = box_list;
        while (temp->next != NULL && strcmp(temp->next->box_name, box->box_name) < 0) {
            temp = temp->next;
        }
		box->next = temp->next;
		temp->next = box;

	}
}

// print the elements of box_list
void print_box_list()
{
    Box_Data* aux;
    for(aux = box_list; aux != NULL; aux = aux->next)
        fprintf(stdout, "%s %zu %zu %zu\n", 
					aux->box_name, 
					aux->box_size, 
					aux->publisher,
					aux->n_subscribers);
}



void destroy_box_list(){
	Box_Data* temp;

	// box list is empty
	if(box_list == NULL){
		return;
	}

	while(box_list->next != NULL){
		temp = box_list;
		box_list = box_list->next;
		free(temp);
	}
	free(box_list);
}



//list the boxes in the server
void list_boxes(char* pipe_name, int register_pipe){
	/* Format message request */
	uint8_t code = REQUEST_BOX_LIST;
	char message_request[BUFFER_SIZE];
	sprintf(message_request, "%c|%s", code, pipe_name);

	/* Send request */
	if (write(register_pipe, message_request, strlen(message_request)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
	}
    close_register_pipe(register_pipe);

	// Open register pipe for writing
    int pipe_i = open(pipe_name, O_RDONLY);
    if (pipe_i == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
	uint8_t last = 0;
	do {
		
		char buffer[BUFFER_SIZE];
		char *box_name;
		memset(buffer, '\0', BUFFER_SIZE);
		ssize_t ret = read(pipe_i, buffer, BUFFER_SIZE);
		if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		Box_Data* curr_box =  (Box_Data*)malloc(sizeof(Box_Data));
		
		strtok(buffer, "|");
		last = (uint8_t) atoi(strtok(NULL, "|"));
		box_name = strtok(NULL, "|");

		
		if (last == 1 && box_name==NULL) {
			fprintf(stdout, "NO BOXES FOUND\n");
			free(curr_box);
			break;
		}
		strcpy(curr_box->box_name, box_name);
		curr_box->box_size  = (long) atoll(strtok(NULL, "|"));
		curr_box->publisher = (long) atoll(strtok(NULL, "|"));
		curr_box->n_subscribers = (long) atoll(strtok(NULL, "|"));

		// no boxes were registered in the mbroker
		insert_box(curr_box);
		
		
	} while (last != 1);

	print_box_list();
	destroy_box_list();
	close(pipe_i);

}


int main(int argc, char **argv) {
	/* Verify and store arguments given */
	verify_arguments(argc);

    char* register_pipe_name = argv[1];
	char pipe_name[32];
	char* action = argv[3];
	strcpy(pipe_name, argv[2]);

	manager_init(pipe_name);

	// melhor fazer isto dentro de cada funcao?????????
	int register_pipe = open_register_pipe(register_pipe_name);

	if (strcmp(action, "create") == 0) {
		char box_name[BOX_SIZE];
		strcpy(box_name, argv[4]);
		create_destroy_box(REQUEST_BOX_CREATE, box_name, pipe_name, register_pipe);
	} else if (strcmp(action, "remove") == 0) {
		char box_name[BOX_SIZE];
		strcpy(box_name, argv[4]);
		create_destroy_box(REQUEST_BOX_REMOVE, box_name, pipe_name, register_pipe);
	} else if (strcmp(action, "list") == 0) {
		list_boxes(pipe_name, register_pipe);
	} else {
		print_usage();
		exit(EXIT_FAILURE);	
	}

	// as soon as it ends a request, the manager terminates


    return -1;
}