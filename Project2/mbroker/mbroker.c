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

#define BUFFER_SIZE 1024

int max_sessions = 0;
int curr_sessions = 0;
int register_pipe; /* pipe opened */



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
    int register_pipee = open(register_pipe_name, O_RDONLY);
    if (register_pipee == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return register_pipee;
}

int register_pub(char *name_pipe, char *box_name) {
	(void) box_name;
	// Open pipe for read
    int pub_pipe = open(name_pipe, O_RDONLY);
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

void process_serialization(char *message) {
	char *args = strtok(message, "|");

	/* Register Publisher */
	if (strcmp(args, "1") == 0) {
		char name_pipe[256];
		char box_name[32];
		strcpy(name_pipe, strtok(NULL, "|"));
		strcpy(box_name, strtok(NULL, "|"));
		int pub_pipe = register_pub(name_pipe, box_name);
		if (pub_pipe != -1) {
			read_publisher(pub_pipe);
		}
		close(pub_pipe);
	} else if (strcmp(args, "2") == 0) {
		char name_pipe[256];
		char box_name[32];
		strcpy(name_pipe, strtok(NULL, "|"));
		strcpy(box_name, strtok(NULL, "|"));
		int pub_pipe = register_sub(name_pipe, box_name);
		if (pub_pipe != -1) {
			//read_publisher(pub_pipe);
		}
		close(pub_pipe);
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
