#include "../fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char file_contents[] = "TESTE1234!";
char const target_path[] = "/f1";
char const link_path1[] = "/l1";
char const link_path2[] = "/l2";


int main() {
    assert(tfs_init(NULL) != -1);

    // test if hardlink is (not) created
    assert(tfs_link(target_path, link_path1) == -1);

    // create new file and write something
    int f = tfs_open(target_path, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));
    assert(tfs_close(f) != -1);


    // create hardlink to target_path
    assert(tfs_link(target_path, link_path1) != -1);

    // create hardlink of hardlink
    assert(tfs_link(link_path1, link_path2) != -1);

    // check if linkpath2 is linked to the correct file
    f = tfs_open(link_path2, 0);
    assert(f != -1);
    char buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);
    assert(strcmp(buffer, file_contents) == 0);
    assert(tfs_close(f) != -1);

    // Truncate the file and check if is truncated
    f = tfs_open(link_path1, TFS_O_TRUNC);
    assert(f != -1);
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);
    assert(tfs_close(f) != -1);

    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}