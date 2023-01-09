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

int main(int argc, char **argv) {
	/* Verify and store arguments given */
	verify_arguments(argc);

    char* register_pipe_name = argv[1];
	char* pipe_name = argv[2];
    char* action = argv[3];

	if (strcmp(action, "create") == 0) {
		char* box_name = argv[4];
			(void) box_name;
	} else if (strcmp(action, "remove") == 0) {
		char* box_name = argv[4];
			(void) box_name;
	} else if (strcmp(action, "list") == 0) {
		// do list
	} else {
		print_usage();
		exit(EXIT_FAILURE);	
	}

	(void) register_pipe_name;
	(void) pipe_name;


    return -1;
}
