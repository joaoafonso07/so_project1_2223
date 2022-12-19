#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <errno.h>

#include "betterassert.h"

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

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
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
 *   - name: absolute path nameir_inode
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t const *root_inode) {
    // TODO: assert that root_inode is the root directory
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

	lock_rd(ROOT_DIR_INUM);
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    int inum = tfs_lookup(name, root_dir_inode);
    size_t offset;

    if (inum >= 0) {
        // The file already exists
		lock_rd(inum);
        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

        if(inode->i_node_type == T_SYMLINK){ // if it is trying to open a soft link it opens the target file
            size_t target_path_size = inode->i_size;
            void* block = data_block_get(inode->i_data_block);
            char target_path[target_path_size];
            strcpy(target_path, block);
            int target_inumber = tfs_lookup(target_path, root_dir_inode);
            if(target_inumber == -1)
                return -1;
			lock_rw(target_inumber);
            inode = inode_get(target_inumber);
            inum = target_inumber;
            ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: soft link target is not an inode");
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
            return -1; // no space in inode table
        }
		lock_rw(inum);
        inode_t *new_inode = inode_get(inum); //added
        new_inode->number_of_hardlinks = 1;  // added

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            return -1; // no space in directory
        }

        offset = 0;
    } else {
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
	unlocks();
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    (void)target;
    (void)link_name;
    // ^ this is a trick to keep the compiler from complaining about unused
    // variables. TODO: remove

	lock_rd(ROOT_DIR_INUM);
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM); //get the inode of the root directory (only one dir)

    int target_inumber = tfs_lookup(target, root_dir_inode); //gets the inumber of the target
    if(target_inumber == -1)
        return -1;

    int new_inumber = inode_create(T_SYMLINK); //creates a new inode for the sym_link (this opperation returns the inumber of the new inode)

    add_dir_entry(root_dir_inode, link_name + 1, new_inumber); //the +1 is to ignore the initial '/'

	lock_rd(new_inumber);
    inode_t* new_inode = inode_get(new_inumber);

    int data_number = new_inode->i_data_block;

    strcpy(data_block_get(data_number), target);

	unlocks();
    return 0;

    //PANIC("TODO: tfs_sym_link");
}

int tfs_link(char const *target, char const *link_name) {
    (void)target;
    (void)link_name;
    // ^ this is a trick to keep the compiler from complaining about unused
    // variables. TODO: remove
	//DIR *opendir(const char *dirpath); //n sei se temos de usar isto?? afonso : acho que não

	lock_rd(ROOT_DIR_INUM);
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM); //get the inode of the root directory (only one dir)

    int target_inumber = tfs_lookup(target, root_dir_inode); //gets the inumber of the target
    if(target_inumber == -1)
        return -1;

    /*Checks if the target is a soft link*/
	lock_rw(target_inumber);
    inode_t *target_inode = inode_get(target_inumber); // gets the inode of the target
    if(target_inode->i_node_type == T_SYMLINK)
        return -1;

    add_dir_entry(root_dir_inode, link_name + 1, target_inumber); // + 1 to ignore the "/"

    target_inode->number_of_hardlinks++; //increments the number of hardlinks

	unlocks();
    return 0;

    //PANIC("TODO: tfs_link");
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

    //  From the open file table entry, we get the inode
	lock_rw(file->of_inumber);
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

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
				unlocks();
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
	unlock_rwl(file->of_inumber);
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    // From the open file table entry, we get the inode
	lock_rd(file->of_inumber);
    inode_t const *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

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
	unlock_rwl(file->of_inumber);
    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    (void)target;
    // ^ this is a trick to keep the compiler from complaining about unused
    // variables. TODO: remove

	lock_rd(ROOT_DIR_INUM);
    inode_t *root_inode = inode_get(ROOT_DIR_INUM); //gets the inode of the root directory
    int target_inumber = tfs_lookup(target, root_inode); // gets the inumber of the target file
    ALWAYS_ASSERT(target_inumber != -1, "tfs_unlink: target file does not exist in TFS")
    lock_rw(target_inumber);
	inode_t *target_inode = inode_get(target_inumber); // gets the inode of the target file

    switch (target_inode->i_node_type)
    {
    case T_FILE:
        target_inode -> number_of_hardlinks--; //reduces the number of hard links of the inode
        clear_dir_entry(root_inode, target + 1); // +1 to ignore the initial "/"
        if(target_inode->number_of_hardlinks == 0)
            inode_delete(target_inumber);
        break;
    case T_SYMLINK:
        inode_delete(target_inumber);
        break;
    case T_DIRECTORY:
        PANIC("tfs_unlink: it is not possible to delete a directory");
    default:
        PANIC("tfs_unlink: unknown file type");
    }

	unlocks();
    return 0;
    //PANIC("TODO: tfs_unlink");
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    (void)source_path;
    (void)dest_path;
    // ^ this is a trick to keep the compiler from complaining about unused
    // variables. TODO: remove

    /* open the source file*/
    FILE *source = fopen(source_path, "r");
    if (source == NULL){ // open fail
        printf("this\n");
        fprintf(stderr, "open error: %s\n", strerror(errno));
        return -1;
    }

    /* open the dest file*/
    int dest = tfs_open(dest_path, TFS_O_CREAT|TFS_O_TRUNC);
    if (dest == -1){// open fail
    fprintf(stderr, "open error: %s\n", strerror(errno));
    return -1;
    }


    char buffer[128];
    memset(buffer, 0, sizeof(buffer)); // set all the buffer index to 0

    /*read the content of the source file and writes it in the dest file*/
    while(true){ //Tests if the end-of-file indicator have reached the EOF
        size_t bytes_read = fread(buffer, sizeof(char), sizeof(buffer), source);
        if (bytes_read){//fread success
            ssize_t bytes_written = tfs_write(dest, buffer, bytes_read);
            if (bytes_written == -1) {
                fprintf(stderr, "write error: %s\n", strerror(errno));
                return -1;
            }

            memset(buffer, 0, sizeof(buffer));
        }
        else{// fread failed
            if (ferror(source)) {
                fprintf(stderr, "read error: %s\n", strerror(errno));
                return -1;
                break; // don't know if it is necessary
            }
            else if (feof(source)) // reached the end of the file (EOF)
                break;
        }
    }

    fclose(source);
    tfs_close(dest);
    return 0;
    //PANIC("TODO: tfs_copy_from_external_fs");
    /*^ before I removed the line above it was not compiling therefore I suppose it must be removed
    (I also think that this is what the comment in the beginning of this function suggests )*/
}
