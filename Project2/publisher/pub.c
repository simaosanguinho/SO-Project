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

// igual ao lab das pipes
void send_msg(int fpub, char const *str) {
    size_t len = strlen(str);
    size_t written = 0;

    while (written < len) {
        ssize_t ret = write(fpub, str + written, len - written);
        if (ret < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        written += (size_t) ret;
    }
}




/* initializes publisher - create publisher named pipe */
void pub_init(char* pipe_name){
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
}


int main(int argc, char **argv) {
	/* Verify and store arguments given */
	verify_arguments(argc);
    char* register_pipe_name = argv[1];
    char pipe_name[256];
	char box_name[32];
	strcpy(pipe_name, argv[2]);
	strcpy(box_name, argv[3]);

	/* create client name_pipe */
	pub_init(pipe_name);

	/* Publisher register request */
	char request_regist[300];
	fprintf(request_regist, "[1|%s|%s]", pipe_name, box_name);

	/* Send request */
	int register_pipe = open(register_pipe_name, O_WRONLY);
	if (register_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    ssize_t written = write(register_pipe, request_regist, sizeof(request_regist));
    assert(written == 1);



    /* Wait for read */
    int fpub = open(pipe_name, O_WRONLY);
    if (fpub == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }





    close(register_pipe);

	/*
	
		verificar se ja ha um publisher ligado

	*/
    
    char* curr_message = NULL;
    // verificar condicao
    while(scanf("%s\n", curr_message) != EOF){
        send_msg(fpub, curr_message);
    }
    



    //tfs_close(pub_box);
    close(fpub);

    //WARN("unimplemented"); // TODO: implement
    return -1;
}

/* TO DO */
/**
 * verify if 
 * 
 */
