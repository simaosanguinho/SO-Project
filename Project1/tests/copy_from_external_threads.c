#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define FILES 5

char *origin_paths[] = {"tests/text1.txt", "tests/text2.txt", "tests/text3.txt", "tests/text4.txt", "tests/text5.txt"};

char *destiny_paths[] = {"/ta1", "/ta2", "/ta3", "/ta4", "/ta5"};

char *expected_texts[] = {
	"Ola Mundo!",
	"Ciao mondo!",
	"Hola mundo!",
	"Dia duit ar domhan!",
	"Konnichiwa sekai!"
};

void *copy_file(void* id) {
	int file_id = *((int*)id);

	int f = tfs_copy_from_external_fs(origin_paths[file_id], destiny_paths[file_id]);
	assert(f != -1);

	return NULL;
}

void test_files() {
	for(int i=0; i<FILES; i++) {
		int f = tfs_open(destiny_paths[i], 0);
		assert(f != -1);
		size_t h = strlen(expected_texts[i]);
		char buffer[strlen(expected_texts[i])];
		ssize_t r = tfs_read(f, buffer, h);
		assert(r == strlen(expected_texts[i]));
		assert(!memcmp(buffer, expected_texts[i], strlen(expected_texts[i])));
	}
}

int main() {
	assert(tfs_init(NULL) != -1);

	pthread_t thread[FILES];
	int thread_args[FILES];

	for(int i=0; i<FILES; i++) {
		thread_args[i] = i;
		assert(pthread_create(&thread[i], NULL, copy_file, (void*)&thread_args[i]) == 0);
	}

	for (int i=0; i<FILES; i++) {
        assert(pthread_join(thread[i], NULL) == 0);
    }

	test_files();

    printf("Successful test.\n");

	return 0;
}