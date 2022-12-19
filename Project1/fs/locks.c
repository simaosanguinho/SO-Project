#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "betterassert.h"

void tfs_rwlock_init(char const *func_name, pthread_rwlock_t *lock) {
	char error[100] = "tfs_rwlock_init: failed to init in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_rwlock_init(lock, NULL) != 0, error);
}

void tfs_rwlock_destroy(char const *func_name, pthread_rwlock_t *lock) {
	char error[100] = "tfs_rwlock_destroy: failed to destroy in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_rwlock_destroy(lock) != 0, error);
}

void tfs_rwlock_rdlock(char const *func_name, pthread_rwlock_t *lock) {
	char error[100] = "tfs_rwlock_rdlock: failed to lock in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_rwlock_rdlock(lock) != 0, error);
}

void tfs_rwlock_wrlock(char const *func_name, pthread_rwlock_t *lock) {
	char error[100] = "tfs_rwlock_wrlock: failed to lock in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_rwlock_wrlock(lock) != 0, error);
}

void tfs_rwlock_unlock(char const *func_name, pthread_rwlock_t *lock) {
	char error[100] = "tfs_rwlock_unlock: failed to unlock in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_rwlock_unlock(lock) != 0, error);
}

void tfs_mutex_init(char const *func_name, pthread_mutex_t *lock) {
	char error[100] = "tfs_mutex_init: failed to init in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_mutex_init(lock, NULL) != 0, error);
}

void tfs_mutex_destroy(char const *func_name, pthread_mutex_t *lock) {
	char error[100] = "tfs_mutex_destroy: failed to destroy in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_mutex_destroy(lock) != 0, error);
}

void tfs_mutex_lock(char const *func_name, pthread_mutex_t *lock) {
	char error[100] = "tfs_mutex_lock: failed to lock in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_mutex_lock(lock) != 0, error);
}

void tfs_mutex_unlock(char const *func_name, pthread_mutex_t *lock) {
	char error[100] = "tfs_mutex_unlock: failed to unlock in ";
	strcat(error, func_name);
	ALWAYS_ASSERT(pthread_mutex_unlock(lock) != 0, error);
}
