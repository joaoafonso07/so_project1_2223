#ifndef STATE_H
#define STATE_H

#include "config.h"
#include "operations.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>


/**
 * Directory entry
 */
typedef struct {
    char d_name[MAX_FILE_NAME];
    int d_inumber;
} dir_entry_t;

typedef enum { T_FILE, T_DIRECTORY, T_SYMLINK} inode_type;

/**
 * Inode
 */
typedef struct {
    inode_type i_node_type;

    size_t i_size;
    int i_data_block;

    int number_of_hardlinks; //not shure of this yet
    // in a more complete FS, more fields could exist here

	pthread_rwlock_t rwl; // read-write lock for inodes
} inode_t;

typedef enum { FREE = 0, TAKEN = 1 } allocation_state_t;

/**
 * Open file entry (in open file table)
 */
typedef struct {
    int of_inumber;
    size_t of_offset;
    pthread_mutex_t open_file_entry_mutex;
} open_file_entry_t;

int state_init(tfs_params);
int state_destroy(void);

size_t state_block_size(void);

int inode_create(inode_type n_type);
void inode_delete(int inumber);
inode_t *inode_get(int inumber);

int clear_dir_entry(inode_t *inode, char const *sub_name);
int add_dir_entry(inode_t *inode, char const *sub_name, int sub_inumber);
int find_in_dir(inode_t const *inode, char const *sub_name);

int data_block_alloc(void);
void data_block_free(int block_number);
void *data_block_get(int block_number);

int add_to_open_file_table(int inumber, size_t offset);
void remove_from_open_file_table(int fhandle);
open_file_entry_t *get_open_file_entry(int fhandle);

void init_rwl(pthread_rwlock_t *rwl);
void init_mutex(pthread_mutex_t *mutex);
void lock_rd(int inumber);
void lock_rw(int inumber);
void lock_mutex(pthread_mutex_t *mutex);
void unlock_rwl(int inumber);
void unlock_mutex(pthread_mutex_t *mutex);
void destroy_rwl(pthread_rwlock_t *rwl);
void destroy_mutex(pthread_mutex_t *mutex);

#endif // STATE_H
