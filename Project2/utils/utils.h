
#ifndef __UTILS_UTILS_H__
#define __UTILS_UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//CONSTANTES

#define BUFFER_SIZE 1024

#define MAXP 256
#define MAXB 32

typedef struct  __attribute__((__packed__)) {
    uint8_t code;
    char pipe_name[MAXP];
    char box_name[MAXB];
} request;

#endif // __UTILS_UTILS_H__
