#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

uint8_t const file_contents[] = "Que informação dramática!";
char const target_path[] = "/f1";

int main() {
    assert(tfs_init(NULL) != -1);
    
    // create a file 
    int fd = tfs_open(target_path, TFS_O_CREAT);
    assert(fd != -1);

    // write in file
    assert(tfs_write(fd, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));
    
    // try to unlink an open file - error expected
    assert(tfs_unlink(target_path) == -1);

    // Close file
    assert(tfs_close(fd) != -1);

    // now that the file is closed - try to unlink it
    assert(tfs_unlink(target_path) == 0);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
