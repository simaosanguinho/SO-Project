#include "operations.h"
#include "config.h"
#include "locks.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#include "betterassert.h"

static pthread_mutex_t tfs_mutex;

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;

    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
    
        return -1;
    }

    tfs_mutex_init(__FUNCTION__, &tfs_mutex);

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }

    tfs_mutex_destroy(__FUNCTION__, &tfs_mutex);
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - inum: the root directory inum
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, int inum) {
    // TODO: assert that root_inode is the root directory
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(inum, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL, "tfs_open: root dir inode must exist");
    tfs_mutex_lock(__FUNCTION__, &tfs_mutex);

    int inum = tfs_lookup(name, ROOT_DIR_INUM);
    size_t offset;

    if (inum >= 0) {
        // The file already exists
        tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);

        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL, "tfs_open: directory files must have an inode");

		if (inode->i_node_type == T_SOFTLINK) {
			char target[FILENAME_MAX];
			void *block = data_block_get(inode->i_data_block);
			ALWAYS_ASSERT(block != NULL, "tfs_open: data block deleted mid-read");
			size_t to_read = inode->i_size;
			memcpy(target, block, to_read);

			return tfs_open(target, mode);
		}

        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
            tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(ROOT_DIR_INUM, name + 1, inum) == -1) {
            tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
            inode_delete(inum);
            return -1; // no space in directory
        }
        tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
        offset = 0;
    } else {
        tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
	if (!valid_pathname(target)) {
        return -1;
    }
    /* a softlink can't point to itself */
    if(!strcmp(target, link_name)){
        return -1;
    }


	inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_sym_link: root dir inode must exist");
    tfs_mutex_lock(__FUNCTION__, &tfs_mutex);

	int inum = inode_create(T_SOFTLINK);
    /* add the inode to directory table */
    if (add_dir_entry(ROOT_DIR_INUM, link_name + 1, inum) == -1) {
        tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
		return -1; // no space in directory
	}
    tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);

	inode_t *inode = inode_get(inum);
	if (inum == -1) {
		return -1; // no space in inode table
	}

	// write target directory size
	size_t to_write = sizeof(target);
    tfs_mutex_lock(__FUNCTION__, &tfs_mutex);
	int bnum = data_block_alloc();
	if (bnum == -1) {
        tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
		return -1; // no space
	}
	inode->i_data_block = bnum;
    tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);


	void *block = data_block_get(inode->i_data_block);
	memcpy(block, target, to_write);
	inode->i_size = to_write;

	return 0;
}

int tfs_link(char const *target, char const *link_name) {
	if (!valid_pathname(target)) {
        return -1;
    }

	inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL, "tfs_link: root dir inode must exist");

    int inum = tfs_lookup(target, ROOT_DIR_INUM);

	if (inum == -1) {
		return -1;
	}

    tfs_mutex_lock(__FUNCTION__, &tfs_mutex);
	inode_t *inode = inode_get(inum);
	if (inode->i_node_type == T_SOFTLINK) {
        tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
		return -1; // can't create hardlink for softlink
	}
	inode->hard_link_count++;

	if (add_dir_entry(ROOT_DIR_INUM, link_name + 1, inum) == -1) {
        tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
		return -1; // no space in directory
	}
    tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
	return 0;
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    tfs_mutex_lock(__FUNCTION__, &file->lock);
    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    tfs_rwlock_wrlock(__FUNCTION__, get_inode_lock(file->of_inumber));

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                tfs_rwlock_unlock(__FUNCTION__, get_inode_lock(file->of_inumber));
                tfs_mutex_unlock(__FUNCTION__, &file->lock);
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    
    tfs_rwlock_unlock(__FUNCTION__, get_inode_lock(file->of_inumber));

    tfs_mutex_unlock(__FUNCTION__, &file->lock);

    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }
    tfs_mutex_lock(__FUNCTION__, &file->lock);
    // From the open file table entry, we get the inode
    inode_t const *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    tfs_rwlock_rdlock(__FUNCTION__, get_inode_lock(file->of_inumber));

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }

    tfs_rwlock_unlock(__FUNCTION__, get_inode_lock(file->of_inumber));

    tfs_mutex_unlock(__FUNCTION__, &file->lock);

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    if (!valid_pathname(target)) {
        return -1; // invalid pathname
    }

    // get root dir inode
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_unlink: root dir inode must exist");
    int inum = tfs_lookup(target, ROOT_DIR_INUM);
    if (inum == -1) {
        return -1; // invalid inode
    }

    // root dir cannot be deleted
    if(inum == 0)
    {
        return -1;
    }

    tfs_mutex_lock(__FUNCTION__, &tfs_mutex);
    inode_t *inode = inode_get(inum);
    if (inode->i_node_type == T_SOFTLINK) {
		if (clear_dir_entry(ROOT_DIR_INUM, target+1) == -1) {
        	return -1; // error deleting the dir entry
    	}
        inode_delete(inum);
        tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
        return 0;
    } else {
		if (is_open(inum) == 1) {
			return -1; // file is opened
		}
		if (inode->hard_link_count == 1) {
			if (clear_dir_entry(ROOT_DIR_INUM, target+1) == -1) {
				return -1; // error deleting the dir entry
			}
			inode_delete(inum);
			tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
			return 0; 
		} else if (inode->hard_link_count > 1) {
			if (clear_dir_entry(ROOT_DIR_INUM, target+1) == -1) {
				return -1; // error deleting the dir entry
			}
			inode->hard_link_count--;
			tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
			return 0;
		}
	}
    tfs_mutex_unlock(__FUNCTION__, &tfs_mutex);
    return -1;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
	FILE *src;
    int dest;
    src = fopen(source_path, "r");
    dest = tfs_open(dest_path,TFS_O_CREAT | TFS_O_TRUNC);

	/* return -1 if src doesn't exist */
    if (src == NULL) {
		tfs_close(dest);
		return -1;
	} 

	/* create a buffer to store source content */
	char buffer[BUFFER_SIZE];
	size_t total_bytes_read = 0;
	size_t bytes_read;
    ssize_t bytes_write;
	do {
		/* read src */
		memset(buffer,0,sizeof(buffer));
		bytes_read = fread(buffer, sizeof(char), sizeof(buffer), src);
		if (bytes_read==-1) {
			fclose(src);
			tfs_close(dest);
			return -1;
		} 
		/* write in dest */
		bytes_write = tfs_write(dest, buffer, bytes_read);
		if (bytes_write == -1 || bytes_write != bytes_read) {
			fclose(src);
			tfs_close(dest);
			return -1;
		}
		total_bytes_read += bytes_read;
	} while (bytes_read >= BUFFER_SIZE*sizeof(char) && 
		total_bytes_read <= state_block_size());

	if (total_bytes_read >= state_block_size()) {
		return -1; /* file bigger than block */
	}

   /* close files */
   fclose(src);
   tfs_close(dest);

   return 0;
}
