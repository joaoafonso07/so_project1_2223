#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

uint8_t const file_contents[] = "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "; // 40 char's
        

char const path1[] = "/f1";
int file; //global file handle


void* thread01(void *p){

    uint8_t buffer[sizeof(file_contents)];
    for(int i = 0; i < 10; i++){
        assert(tfs_read(file, buffer, sizeof(buffer)) == sizeof(file_contents));
        assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);
        memset(buffer, 0, sizeof(buffer));
    }
    return p;
}

void* thread02(void *p){

    uint8_t buffer[sizeof(file_contents)];
    for(int i = 0; i < 10; i++){
        assert(tfs_read(file, buffer, sizeof(buffer)) == sizeof(file_contents));
        assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);
        memset(buffer, 0, sizeof(buffer));
    }
    return p;
}

void* thread03(void *p){

    uint8_t buffer[sizeof(file_contents)];
    for(int i = 0; i < 10; i++){
        assert(tfs_read(file, buffer, sizeof(buffer)) == sizeof(file_contents));
        assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);
        memset(buffer, 0, sizeof(buffer));
    }
    return p;
}


void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

int main(){
    assert(tfs_init(NULL) != -1);

    file = tfs_open(path1, TFS_O_CREAT);
    assert(file != -1);
    
    assert_empty_file(path1); // sanity check

    assert(tfs_write(file, file_contents, sizeof(file_contents)) == sizeof(file_contents));

    pthread_t tid[3];
    
        
    assert(pthread_create(&tid[0], NULL, thread01, NULL) == 0);
    assert(pthread_create(&tid[1], NULL, thread02, NULL) == 0);
    assert(pthread_create(&tid[2], NULL, thread03, NULL) == 0);

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);

    assert(tfs_close(file) != -1);
    assert(tfs_destroy() != -1);
    printf("Successful test if no ThreadSanitizer warnings.\n");
	
	return 0;
}