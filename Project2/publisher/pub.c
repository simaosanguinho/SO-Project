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
    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
}

/* verify arguments */
int verify_arguments(int argc, char* argv[]){

    if(argc != 4){
        print_usage();
        return -1;
    }
    return 0;
}

// igual ao lab das pipes
void send_msg(int fpub, char const *str) {
    size_t len = strlen(str);
    size_t written = 0;

    while (written < len) {
        ssize_t ret = write(tx, str + written, len - written);
        if (ret < 0) {
            fprintf(stderr, "[ERR]: write failed: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        written += ret;
    }
}




/* initializes publisher - create an open the register pipe */
int pub_init(char* pipe_name){

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

    // Open pipe for writing
    int fpub = open(pipe_name, O_WRONLY);
    if (fpub == -1) {
        fprintf(stderr, "[ERR]: open failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fpub;
}


int main(int argc, char **argv) {
    
    //TEST ARGUMENTS
    verify_arguments(argc, argv);

    char* register_pipe_name = argv[1];
    char* pipe_name = argv[2];
    char* box_name = argv[3];

    int fpub = pub_init(pipe_name);
    int pub_box = tfs_open(box_name, TFS_O_APPEND);

    
    char* curr_message = NULL;
    // verificar condicao
    while(scanf("%s\n", curr_message) != EOF){
        send_msg(fpub, curr_message);
    }
    

    



    tfs_close(pub_box);
    close(fpub);

    //WARN("unimplemented"); // TODO: implement
    return -1;
}

/* TO DO */
/**
 * verify if 
 * 
 */
