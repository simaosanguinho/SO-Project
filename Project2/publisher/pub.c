/**
 * @file pub.c
 * @author Simão Sanguinho - ist1102082, Henrique Pimentel ist1104156
 * @group al041
 * @date 2022-01-15
 */


#include "logging.h"
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
#include "utils.h"
#include <signal.h>

char pipe_name[PIPE_SIZE];
int pipe_i = -1;

void print_usage(){
    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
}

/* verify arguments */
int verify_arguments(int argc){
    if(argc != 4){
        print_usage();
        exit(EXIT_FAILURE);
    }
    return 0;
}

/* Send register code */
void register_publisher(char *register_pipe_name, char box_name[BOX_SIZE]) {
	/* Format message request */
	MessageRequest request;
	request.code = REQUEST_PUB_REGISTER;
	memset(request.pipe_name, '\0', sizeof(request.pipe_name));
	memset(request.box_name, '\0', sizeof(request.box_name));
	memcpy(request.pipe_name, pipe_name, strlen(pipe_name));
	memcpy(request.box_name, box_name, strlen(box_name));

	/* Send request */
	int register_pipe = open(register_pipe_name, O_WRONLY);
	if (register_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (write(register_pipe, &request, sizeof(request)) == -1) {
        fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
	}
    close(register_pipe);
}


/* read stdin and send it to mbroker, until reaches EOF or mbroker closes pipe */
void send_messages() {
	char message_buffer[MESSAGE_SIZE];

	while (true) {
		memset(message_buffer, '\0', MESSAGE_SIZE);

		if (fgets(message_buffer, sizeof(message_buffer), stdin) == NULL) {
			break; /* EOF */
		}

		size_t newline_index = strcspn(message_buffer, "\n");
		message_buffer[newline_index] = '\0';
		MessageRequest request;
		request.code = PUB_WRITE_MSG;
		memset(request.message, '\0', sizeof(request.message));
		memcpy(request.message, message_buffer, strlen(message_buffer));

		// write message in pipe
		ssize_t written = write(pipe_i, &request, sizeof(request));
		if (written < 0) {
			exit(EXIT_FAILURE);
		} else if (written == 0) { /* mbroker closed session */
			return;
		}
		
	}
}

/* initializes publisher - create publisher named pipe */
void pub_init(){
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
	exit(EXIT_SUCCESS);
}


int main(int argc, char **argv) {
	/* Verify and store arguments given */
	verify_arguments(argc);
    char* register_pipe_name = argv[1];
	char box_name[BOX_SIZE];
	strcpy(pipe_name, argv[2]);
	strcpy(box_name, argv[3]);

	/* create client name_pipe */
	pub_init();

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		exit(EXIT_FAILURE);
	} else if (signal(SIGQUIT, sig_handler) == SIG_ERR) {
		exit(EXIT_FAILURE);
	}

	register_publisher(register_pipe_name, box_name);

    /* Wait for mbroker to read pipe */
    pipe_i = open(pipe_name, O_WRONLY);
    if (pipe_i == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

	send_messages();
	unlink(pipe_name);
    close(pipe_i);
    return -1;
}
