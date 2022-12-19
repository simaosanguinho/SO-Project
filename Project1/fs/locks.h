#ifndef LOCKS_H
#define LOCKS_H

#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>

void tfs_rwlock_init(char const *func_name, pthread_rwlock_t *lock);

void tfs_rwlock_destroy(char const *func_name, pthread_rwlock_t *lock);

void tfs_rwlock_rdlock(char const *func_name, pthread_rwlock_t *lock);

void tfs_rwlock_wrlock(char const *func_name, pthread_rwlock_t *lock);

void tfs_rwlock_unlock(char const *func_name, pthread_rwlock_t *lock);

void tfs_mutex_init(char const *func_name, pthread_mutex_t *lock);

void tfs_mutex_destroy(char const *func_name, pthread_mutex_t *lock);

void tfs_mutex_lock(char const *func_name, pthread_mutex_t *lock);

void tfs_mutex_unlock(char const *func_name, pthread_mutex_t *lock);


#endif
