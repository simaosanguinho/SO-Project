#include "logging.h"
#include "operations.h"
#include "utils.h"
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

int messages_received = 0;
char pipe_name[256];
int pipe_i = -1;

void print_usage(){
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
}

/* verify arguments */
int verify_arguments(int argc){
    if(argc != 4){
        print_usage();
        exit(EXIT_FAILURE);
    }
    return 0;
}


void register_subscriber(char *register_pipe_name, char box_name[32]) {
	/* Format message request */
	uint8_t code = REQUEST_SUB_REGISTER;
	char message_request[BUFFER_SIZE];
	sprintf(message_request, "%c|%s|%s", code, pipe_name, box_name);

	/* Send request */
	int register_pipe = open(register_pipe_name, O_WRONLY);
	if (register_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (write(register_pipe, message_request, strlen(message_request)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
	}
	close(register_pipe);
}


void read_messages() {
	while (true) {
		char buffer[BUFFER_SIZE];
		memset(buffer, '\0', BUFFER_SIZE);
		ssize_t ret = read(pipe_i, buffer, BUFFER_SIZE);
		printf("BUFFER HAS: %s\n", buffer);
		if (ret == -1) {
			fprintf(stderr, "[ERR]: read failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		} else if (ret==0) {
			break;
		} else {
			buffer[ret] = 0;
			// extract the message code
			strtok(buffer, "|");
			// print the message
			fprintf(stdout, "%s\n", strtok(NULL, "|"));
			messages_received++;
		}
	}
}

// create the session pipe
void sub_init() {
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

/* Removes client's pipe from file system */
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
	if (pipe_i != -1) {
		close(pipe_i);
	}
	if (unlink(pipe_name) != 0) {
        exit(EXIT_FAILURE);
    }
	fprintf(stdout, "%d\n", messages_received);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	/* Verify and store arguments given */
	verify_arguments(argc);
    char* register_pipe_name = argv[1];
	char box_name[32];
	strcpy(pipe_name, argv[2]);
	strcpy(box_name, argv[3]);

	/* create client name_pipe */
    sub_init();

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		exit(EXIT_FAILURE);
	} else if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
		exit(EXIT_FAILURE);
	}

	register_subscriber(register_pipe_name, box_name);

	/* Wait for mbroker to write pipe */
	pipe_i = open(pipe_name, O_RDONLY);
    if (pipe_i == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
	read_messages();
	fprintf(stdout, "%d\n", messages_received);
    close(pipe_i);
	unlink(pipe_name);

    return -1;
}
