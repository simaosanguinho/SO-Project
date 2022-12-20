#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

int main() {

    /* filename and link name is the same */
    uint8_t const file_contents[] = "Hello World!";
    const char *file_path = "/file1";
    const char *link_path1 = "/file1";
    const char *link_path2 = "/ks";


    assert(tfs_init(NULL) != -1);

    // create a file 
    int fd = tfs_open(file_path, TFS_O_CREAT);
    assert(fd != -1);

    // write in file
    assert(tfs_write(fd, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    // Immediately close file
    assert(tfs_close(fd) != -1);

    // Create symbolic link - can't do such as the filename is the same
    assert(tfs_sym_link(file_path, link_path1) == -1);

    // Link unusable - was not created
    //assert(tfs_open(link_path1, TFS_O_APPEND) == -1);

    // try creating another sym link to the file with a different name
    assert(tfs_sym_link(file_path, link_path2) != -1);

    // check if we can access the file through the second sym link
    fd = tfs_open(link_path2, 0);
    assert(fd != -1);
    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(fd, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);
    assert(tfs_close(fd) != -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");
}