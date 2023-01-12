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
void create_destroy_box(uint8_t code, char* box_name, char* pipe_name, int register_pipe){
	/* Format message request */
	char message_request[BUFFER_SIZE];
	sprintf(message_request, "%c|%s|%s", code, pipe_name, box_name);

	/* Send request */
	if (write(register_pipe, message_request, strlen(message_request)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
	}
    close_register_pipe(register_pipe);
	int pipe = open(pipe_name, O_RDONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	char buffer[BUFFER_SIZE];
	char error_message[BUFFER_SIZE];
	int32_t return_code;

	ssize_t ret = read(pipe, buffer, BUFFER_SIZE);
	if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
	}

	char *args = strtok(buffer, "|");
			return_code = (int32_t) atoi(strtok(NULL, "|"));
			strcpy(error_message, strtok(NULL, "|"));

	// box was successfully created or destroyed
	if(return_code != -1)
		fprintf(stdout, "OK\n");
	 
	else
		fprintf(stdout, "ERROR %s\n", error_message);
	
	close(pipe);

}



// SIMAO - USAR LISTA LIGADA PARA ORDENAR AS BOXES


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
    int pipe = open(pipe_name, O_RDONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	do {
		uint8_t last;
		char buffer[BUFFER_SIZE];
		char box_name[32];
		uint64_t box_size[64];
		uint64_t npublishers, n_subscribers;

		memset(buffer, '\0', BUFFER_SIZE);
		ssize_t ret = read(pipe, buffer, BUFFER_SIZE);
		if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		} else {
			char *args = strtok(buffer, "|");
			strcpy(last , strtok(NULL, "|"));
			strcpy(box_name, strtok(NULL, "|"));
			box_size = (uint64_t) atoll(strtok(NULL, "|"));
			strcpy(box_size, strtok(NULL, "|"));
			strcpy(n_publishers, strtok(NULL, "|"));
			strcpy(n_subscribers, strtok(NULL, "|"));

			// no boxes were registered in the mbroker
			if (strlen(box_name) == 0) {
				fprintf(stdout, "NO BOXES FOUND\n");
			}
			
			fprintf(stdout, "%s %zu %zu %zu\n", 
						box_name, box_size, n_publishers, n_subscribers);
		}
		
	} while (last !='1');

	// reopen the pipe
	// read the list of boxes
	// print them
	//close pipe
	close(pipe);

}



int main(int argc, char **argv) {
	/* Verify and store arguments given */
	verify_arguments(argc);

    char* register_pipe_name = argv[1];
	char* pipe_name = argv[2];
    char* action = argv[3];

	manager_init(pipe_name);

	// melhor fazer isto dentro de cada funcao?????????
	int register_pipe = open_register_pipe(register_pipe_name);

	if (strcmp(action, "create") == 0) {
		char* box_name = argv[4];
		create_destroy_box('3', box_name, pipe_name, register_pipe);
	} else if (strcmp(action, "remove") == 0) {
		char* box_name = argv[4];
		request_box('5', box_name, pipe_name, register_pipe);
	} else if (strcmp(action, "list") == 0) {
		list_boxes(pipe_name, register_pipe);
	} else {
		print_usage();
		exit(EXIT_FAILURE);	
	}

	// as soon as it ends a request, the manager terminates

    return -1;
}