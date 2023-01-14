#include "logging.h"
#include "utils.h"
#include "operations.h"
#include "producer-consumer.h"
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
#include <pthread.h>

// pcq_create: create a queue, with a given (fixed) capacity
//
// Memory: the queue pointer must be previously allocated
// (either on the stack or the heap)
int pcq_create(pc_queue_t *queue, size_t capacity) {

    if (!(queue->pcq_buffer = (void **) malloc(capacity * sizeof(void *)))) {
		return -1;
	}

	queue->pcq_capacity = capacity;

	/* init pcq_current_size_lock */
	if (pthread_mutex_init(&queue->pcq_current_size_lock, NULL) != 0) {
		return -1;
	}
	queue->pcq_current_size = 0;
	
	/* init pcq_head_lock */
	if (pthread_mutex_init(&queue->pcq_head_lock, NULL) != 0) {
		pthread_mutex_destroy(&queue->pcq_current_size_lock);
		return -1;
	}
	queue->pcq_head = 0;

	/* init pcq_tail_lock */
	if (pthread_mutex_init(&queue->pcq_tail_lock, NULL) != 0) {
		pthread_mutex_destroy(&queue->pcq_head_lock);
		pthread_mutex_destroy(&queue->pcq_current_size_lock);
		return -1;
	}
	queue->pcq_tail = 0;

	/* init pcq_pusher_condvar_lock */
	if (pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL) != 0) {
		pthread_mutex_destroy(&queue->pcq_tail_lock);
		pthread_mutex_destroy(&queue->pcq_head_lock);
		pthread_mutex_destroy(&queue->pcq_current_size_lock);
		return -1;
	}
	if (pthread_cond_init(&queue->pcq_pusher_condvar, NULL) != 0) {
		pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
		pthread_mutex_destroy(&queue->pcq_tail_lock);
		pthread_mutex_destroy(&queue->pcq_head_lock);
		pthread_mutex_destroy(&queue->pcq_current_size_lock);
		return -1;
	}

	/* init pcq_popper_condvar_lock */
	if (pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL) != 0) {
		pthread_cond_destroy(&queue->pcq_pusher_condvar);
		pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
		pthread_mutex_destroy(&queue->pcq_tail_lock);
		pthread_mutex_destroy(&queue->pcq_head_lock);
		pthread_mutex_destroy(&queue->pcq_current_size_lock);
		return -1;
	}
	if (pthread_cond_init(&queue->pcq_popper_condvar, NULL) != 0) {
		pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
		pthread_cond_destroy(&queue->pcq_pusher_condvar);
		pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
		pthread_mutex_destroy(&queue->pcq_tail_lock);
		pthread_mutex_destroy(&queue->pcq_head_lock);
		pthread_mutex_destroy(&queue->pcq_current_size_lock);
		return -1;
	}

	return 0;
}

// pcq_destroy: releases the internal resources of the queue
//
// Memory: does not free the queue pointer itself
int pcq_destroy(pc_queue_t *queue) {
	int error = 0;
	free(queue->pcq_buffer);
	error += pthread_cond_destroy(&queue->pcq_popper_condvar);
	error += pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
	error += pthread_cond_destroy(&queue->pcq_pusher_condvar);
	error += pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
	error += pthread_mutex_destroy(&queue->pcq_tail_lock);
	error += pthread_mutex_destroy(&queue->pcq_head_lock);
	error += pthread_mutex_destroy(&queue->pcq_current_size_lock);
	if (error==0) {
		return 0;
	}
	return -1;
}

// pcq_enqueue: insert a new element at the front of the queue
//
// If the queue is full, sleep until the queue has space
int pcq_enqueue(pc_queue_t *queue, void *elem) {
	pthread_mutex_lock(&queue->pcq_pusher_condvar_lock);
	while (queue->pcq_current_size == queue->pcq_capacity) {
		pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_pusher_condvar_lock);
	}

	pthread_mutex_lock(&queue->pcq_head_lock); /* lock head of queue */
	queue->pcq_buffer[queue->pcq_head] = elem; /* add element to buffer head */
	queue->pcq_head = (queue->pcq_head+1)%queue->pcq_capacity;

	pthread_mutex_lock(&queue->pcq_current_size_lock); /* lock current_size modification */
	queue->pcq_current_size++;
	pthread_mutex_unlock(&queue->pcq_current_size_lock);

	pthread_mutex_unlock(&queue->pcq_head_lock);
	pthread_cond_signal(&queue->pcq_popper_condvar); /* send signal to dequeue */
	pthread_mutex_unlock(&queue->pcq_pusher_condvar_lock);

	return 0;
}

// pcq_dequeue: remove an element from the back of the queue
//
// If the queue is empty, sleep until the queue has an element
void *pcq_dequeue(pc_queue_t *queue) {
	void *elem;

	pthread_mutex_lock(&queue->pcq_popper_condvar_lock);
	// lock while the queue is empty - cannot pop
	while (queue->pcq_current_size == 0) {
		pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_popper_condvar_lock);
	}

	pthread_mutex_lock(&queue->pcq_tail_lock); /* lock tail of queue */
	elem = queue->pcq_buffer[queue->pcq_tail]; /* copy element from buffer tail */
	queue->pcq_tail = (queue->pcq_tail+1)%queue->pcq_capacity;

	pthread_mutex_lock(&queue->pcq_current_size_lock); /* lock current_size modification */
	queue->pcq_current_size--;
	pthread_mutex_unlock(&queue->pcq_current_size_lock);

	pthread_mutex_unlock(&queue->pcq_tail_lock);
	pthread_cond_signal(&queue->pcq_pusher_condvar); /* send signal to queue */
	pthread_mutex_unlock(&queue->pcq_popper_condvar_lock);

	return elem;
}
