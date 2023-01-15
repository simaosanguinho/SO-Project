/**
 * @file manager.c
 * @author Simão Sanguinho - ist1102082, Henrique Pimentel ist1104156
 * @group al041
 * @date 2022-01-15
 */

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
	uint64_t box_size;
	uint64_t n_subscribers;
	uint64_t publisher;
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
	MessageRequest request;
	request.code = code;
	memset(request.pipe_name, '\0', sizeof(request.pipe_name));
	memset(request.box_name, '\0', sizeof(request.box_name));

	memcpy(request.pipe_name, pipe_name, strlen(pipe_name));
	memcpy(request.box_name, box_name, strlen(box_name));

	/* Send request */
	if (write(register_pipe, &request, sizeof(request)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
	}
    close_register_pipe(register_pipe);
	int pipe_i = open(pipe_name, O_RDONLY);
    if (pipe_i == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	MessageRequest response;

	ssize_t ret = read(pipe_i, &response, sizeof(response));
	if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
	}

	// box was successfully created or destroyed
	if (response.return_code != -1) {
		fprintf(stdout, "OK\n");
	} else {
		fprintf(stdout, "ERROR %s\n", response.message);
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
        fprintf(stdout, "%s %llu %llu %llu\n", 
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
	MessageRequest request;
	request.code = REQUEST_BOX_LIST;
	memset(request.pipe_name, '\0', sizeof(request.pipe_name));
	memcpy(request.pipe_name, pipe_name, strlen(pipe_name));

	/* Send request */
	if (write(register_pipe, &request, sizeof(request)) == -1) {
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
	MessageRequest response;
	do {
		ssize_t ret = read(pipe_i, &response, sizeof(response));
		if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		Box_Data* curr_box =  (Box_Data*)malloc(sizeof(Box_Data));
		
		if (response.last == 1 && strlen(response.box_name) == 0) {
			fprintf(stdout, "NO BOXES FOUND\n");
			free(curr_box);
			break;
		}
		strcpy(curr_box->box_name, response.box_name);
		curr_box->box_size  = response.box_size;
		curr_box->publisher = response.n_publishers;
		curr_box->n_subscribers = response.n_subscribers;

		// no boxes were registered in the mbroker
		insert_box(curr_box);
		
		
	} while (response.last != 1);

	print_box_list();
	destroy_box_list();
	close(pipe_i);

}


int main(int argc, char **argv) {
	/* Verify and store arguments given */
	verify_arguments(argc);

    char* register_pipe_name = argv[1];
	char pipe_name[PIPE_SIZE];
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