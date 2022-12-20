#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define THREADS 30

char *origin_paths = "tests/text1.txt";

char *destiny_path = "/ta1";

char *expected_text = "Ola Mundo!";


void *read_file(void *nul) {
    (void)nul;


	int f = tfs_open(destiny_path, 0);
	assert(f != -1);
	char buffer[strlen(expected_text)];
	assert(tfs_read(f, buffer, strlen(expected_text)) == strlen(expected_text));
	assert(!memcmp(buffer, expected_text, strlen(expected_text)));

    assert(tfs_close(f) == 0);

    return NULL;
}



int main() {
    assert(tfs_init(NULL) != -1);

    pthread_t threads[THREADS];

	assert(tfs_copy_from_external_fs(origin_paths, destiny_path) == 0);

    for (int i=0; i<THREADS; i++) {
        assert(pthread_create(&threads[i], NULL, read_file, NULL) == 0);
    }

    for (int i=0; i<THREADS; i++) {
        assert(pthread_join(threads[i], NULL) == 0);
    }

    assert(tfs_destroy() == 0);
    printf("Successful test.\n");

    return 0;
}