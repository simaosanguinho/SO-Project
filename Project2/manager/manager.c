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


static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    //print_usage();
    //WARN("unimplemented"); // TODO: implement

    //char* register_pipe_name = argv[1];
    //char* pipe_name = argv[2];
    char* action = argv[3];


    switch(argc){    
        case 5:  
            //char* box_name = argv[4];  
            if(!strcmp(action, "create")){
                // create
                break;
            }
            if(!strcmp(action, "remove")){
                // remove
                break;
            }
            break;   
        case 4:   
            if(!strcmp(action, "list")){
                // list boxes
                break;
            }    
            break;    

        // possible error
        default:
            print_usage();

    }   


    return -1;
}
