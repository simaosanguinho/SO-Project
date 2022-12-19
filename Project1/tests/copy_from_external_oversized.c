#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    char *path_copied_file = "/f1";
    char *path_src = "tests/oversized_file.txt";

    assert(tfs_init(NULL) != -1);

    int f;


    f = tfs_copy_from_external_fs(path_src, path_copied_file);
	/* only continues if the error was detected */
    assert(f == -1);

	assert(tfs_destroy(NULL) != -1);

    printf("Successful test.\n");

    return 0;
}
