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


/* returns file descriptor of pipe that represents the manager */
int manager_init(char* register_pipe_name, char* pipe_name, char* box_name){

    // Remove pipe if it does not exist
    if (unlink(pipe_name) != 0) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipe_name,
        strerror(errno));
        exit(EXIT_FAILURE);
    }
    // Create pipe
    if (mkfifo(pipe_name, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
	// Open pipe for reading
    int pipe = open(pipe_name, O_RDONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return pipe;
}

// close client's pipe
int manager_destroy(int pipe){
	close(pipe);
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

close_register_pipe(int register_pipe){
	close(register_pipe);
}




int request_create_box(char* box_name, char* pipe_name, int register_pipe){


	// create request
    request register_request;
    register_request.code = 3;
    strcpy(register_request.pname, pipe_name);
    strcpy(register_request.bname, box_name);

	 // write request in the pipe
    ssize_t written = write(register_pipe, (void*)&register_request, sizeof(register_request));
    
    // close pipe
    close_register_pipe(register_pipe);


}

int request_remove_box(char* box_name, char* pipe_name, int register_pipe){
	
	// CODIGO DUPLICADO - ADD ASBTRACAO

	// create request
    request register_request;
    register_request.code = 5;
    strcpy(register_request.pname, pipe_name);
    strcpy(register_request.bname, box_name);

	 // write request in the pipe
    ssize_t written = write(register_pipe, (void*)&register_request, sizeof(register_request));
    
    // close pipe
    close_register_pipe(register_pipe);

}


void list_boxes(char* pipe_name, int register_pipe){

	// create request
    request register_request;
    register_request.code = 7;
    strcpy(register_request.pname, pipe_name);

	 // write request in the pipe
    ssize_t written = write(register_pipe, (void*)&register_request, sizeof(register_request));
    
    // close pipe
    close_register_pipe(register_pipe);


	// WAIT THE REQUEST TO BE ANSWERED 

	// reopen the pipe
	// read the list of boxes
	// print them
	//close pipe

}



int main(int argc, char **argv) {
	/* Verify and store arguments given */
	verify_arguments(argc);

    char* register_pipe_name = argv[1];
	char* pipe_name = argv[2];
    char* action = argv[3];

	int pipe = manager_init(register_pipe_name, pipe_name, box_name);

	// melhor fazer isto dentro de cada funcao?????????
	int register_pipe = open_register_pipe(register_pipe_name);

	
	if (strcmp(action, "create") == 0) {
		char* box_name = argv[4];
			request_create_box(box_name, pipe_name, register_pipe);
	} else if (strcmp(action, "remove") == 0) {
		char* box_name = argv[4];
			request_remove_box(box_name, pipe_name, register_pipe);
	} else if (strcmp(action, "list") == 0) {
			list_boxes(pipe_name, register_pipe);
	} else {
		print_usage();
		exit(EXIT_FAILURE);	
	}

	manager_destroy(pipe);


    return -1;
}