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
int verify_arguments(int argc, char* argv[]){

    if(argc != 3){
        print_usage();
        return -1;
    }
    return 0;
}

/* returns file desciptor of pipe that represents mbroker*/
int mbroker_init(char* register_pipe_name, int max_s){

    int f;
    max_sessions = max_s;
    
        // Remove pipe if it does not exist
    if (unlink(register_pipe_name) != 0) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", pipe_name,
        strerror(errno));
        exit(EXIT_FAILURE);
    }

     // Create pipe
    if (mkfifo(register_pipe_name, 0640) != 0) {
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



int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    // POR ISTO NUMA FUNCAO PRINTF_USAGE
    //fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
    //WARN("unimplemented"); // TODO: implement

    // initialise file system
    assert(tfs_init(NULL) != -1);

    

    char* register_pipe_name = argv[1];
    int max_sessions = argv[2];

    int register_pipe = mbroker_init(register_pipe_name, max_sessions);
    

    close(register_pipe);

    return -1;
}
