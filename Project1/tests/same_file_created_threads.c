#include "../fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define N_THREADS 4
#define FILENAME "/f1"

const char content[] = "X";


void *create_file(void *arg) {
    (void)arg;

    int *f = malloc(sizeof(int));
    assert(f != NULL);
    *f = tfs_open(FILENAME, TFS_O_CREAT | TFS_O_APPEND);
    assert(*f != -1);

    assert(tfs_write(*f, content, 1) == 1);

    return f;
}



int main() {

    assert(tfs_init(NULL) != -1);

    int *fd[N_THREADS];

    pthread_t thread_id[N_THREADS];
    char buffer[N_THREADS + 1];

    /* create threads */
    for (int i = 0; i < N_THREADS; i++) 
        assert(pthread_create(&thread_id[i], NULL, create_file, NULL) == 0); 
   
    for (int i = 0; i < N_THREADS; i++) 
        pthread_join(thread_id[i], NULL);

    // close the files 
    for(int i = 0; i<N_THREADS; ++i){
        tfs_close(*fd[i]);
    }
   
    int f = tfs_open(FILENAME, TFS_O_CREAT);
    assert(f != -1);
    ssize_t r = tfs_read(f, buffer, N_THREADS + 1);
    assert(r == N_THREADS);
    assert(tfs_close(f) != -1);

    tfs_destroy();
    printf("Successful test.\n");

    return 0;

}

