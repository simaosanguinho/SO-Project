/**
 * @file utils.h
 * @author Simão Sanguinho - ist1102082, Henrique Pimentel ist1104156
 * @group al041
 * @date 2022-01-15
 */

#ifndef __UTILS_UTILS_H__
#define __UTILS_UTILS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

//CONSTANTES

#define BUFFER_SIZE 1342

#define MESSAGE_SIZE 1024
#define PIPE_SIZE 256
#define BOX_SIZE 32

typedef struct __attribute__((packed)) message_request {
	uint8_t code;
	char pipe_name[PIPE_SIZE];
	char box_name[BOX_SIZE];
	int32_t return_code;
	char message[MESSAGE_SIZE];
	uint8_t last;
	uint64_t box_size;
	uint64_t n_publishers;
	uint64_t n_subscribers;
} MessageRequest;


/* Protocol constants */
#define REQUEST_PUB_REGISTER    1
#define REQUEST_SUB_REGISTER    2
#define REQUEST_BOX_CREATE      3
#define ANSWER_BOX_CREATE       4
#define REQUEST_BOX_REMOVE      5
#define ANSWER_BOX_REMOVE       6
#define REQUEST_BOX_LIST        7
#define ANSWER_BOX_LIST         8
#define PUB_WRITE_MSG           9
#define SUB_READ_MSG            10




#endif // __UTILS_UTILS_H__
