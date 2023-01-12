
#ifndef __UTILS_UTILS_H__
#define __UTILS_UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//CONSTANTES

#define BUFFER_SIZE 1024

#define MAXP 256
#define MAXB 32


/* Protocol constants */
#define REQUEST_PUB_REGISTER    '1'
#define REQUEST_SUB_REGISTER    '2'
#define REQUEST_BOX_CREATE      '3'
#define ANSWER_BOX_CREATE       '4'
#define REQUEST_BOX_REMOVE      '5'
#define ANSWER_BOX_REMOVE       '6'
#define REQUEST_BOX_LIST        '7'
#define ANSWER_BOX_LIST         '8'
#define PUB_WRITE_MSG           '9'
#define SUB_READ_MSG            '10'

typedef struct  __attribute__((__packed__)) {
    uint8_t code;
    char pipe_name[MAXP];
    char box_name[MAXB];
} request;




#endif // __UTILS_UTILS_H__
