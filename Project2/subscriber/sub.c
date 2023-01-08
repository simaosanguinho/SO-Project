#include "logging.h"
#include "operations.h"
#include "utils.c"
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

void print_usage(){
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
}

/* verify arguments */
int verify_arguments(int argc, char* argv[]){

    if(argc != 4){
        print_usage();
        return -1;
    }
    return 0;
}

void send_register_request(char* register_pipe_name, char* pipe_name, char* box_name){

    // create request
    request register_request;
    register_request.code = 2;
    strcpy(register_request.pname, pipe_name);
    strcpy(register_request.bname, box_name);

     // Open register pipe for reading
    int register_pipe = open(register_pipe_name, O_WRONLY);
    if (register_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // write request in the pipe
    size_t written = write(register_pipe, register_request, sizeof(register_request));
    assert(written == 1);

    // close pipe
    close(register_pipe);

}


/* returns file descriptor of pipe that represents sub*/
int sub_init(char* register_pipe_name, char* pipe_name, char* box_name){

    // send a request to be registered in the server
    send_register_request(register_pipe_name, pipe_name, box_name);

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
    int register_pipe = open(pipe_name, O_RDONLY);
    if (register_pipe == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return register_pipe;
}

int read_past_messages() {
    int f = tfs_open(box_name, 0);
    char *buffer;
    if (f==-1) {return -1;}
    tfs_read(f, buffer)
}


int main(int argc, char **argv) {
    
    char* register_pipe_name = argv[1];
    char* pipe_name = argv[2];
    char* box_name = argv[3];

    //verify arguments
    verify_arguments(argc, argv);

    //initialise client
    sub_init(register_pipe_name, pipe_name, box_name);

    //read past messages


    // read new messages
    while(true) {
        
    };




    close(fsub);

    return -1;
}
