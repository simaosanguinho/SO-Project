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

int max_sessions = 0;
int curr_sessions = 0;

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

/* returns file desciptor of pipe that represents mbroker*/
int mbroker_init(char* register_pipe_name){
    
	// Remove pipe if it does not exist
    if (unlink(register_pipe_name) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", register_pipe_name,
        strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Create pipe
    if (mkfifo(register_pipe_name, 0640) != 0) {
        fprintf(stderr, "[ERR]: mkfifo failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Open pipe for reading
    int register_pipe = open(register_pipe_name, O_RDONLY);
    if (register_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return register_pipe;
}

void process_serialization(char *message) {
	char *args = strtok(argsStr, "|");
	if (strcmp(args[0], "1") == 0) {
		char name_pipe[256];
		char box_name[32];
		strcpy(name_pipe, args[1]);
		strcpy(box_name, args[2]);
		fprintf(stderr, "[RECEIVED]: %s %s\n", name_pipe, box_name);
	}

}


int main(int argc, char **argv) {
    // initialise file system
    assert(tfs_init(NULL) != -1);

	/* Verify and store arguments given */
    verify_arguments(argc);
    char* register_pipe_name = argv[1];
    max_sessions = atoi(argv[2]);

    int register_pipe = mbroker_init(register_pipe_name);
    
    while (true) {
        char buffer[500];
        ssize_t ret = read(register_pipe, buffer, 500 - 1);
        if (ret == -1) {
            // ret == -1 indicates error
            fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        } else if (ret != 0) {
			buffer[ret] = 0;
			fprintf(stderr, "[RECEIVED]: %s\n", buffer);
			process_serialization(buffer);
		}
    }

    return -1;
}