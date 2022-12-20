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

uint8_t const file_contents1[] = 
		"BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! ";
uint8_t const file_contents2[] = "BBB";
char const path1[] = "/f1";
int checker = 0;

void* thread01(void *p){
	int f = tfs_open(path1, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents1, sizeof(file_contents1)) == sizeof(file_contents1));

    assert(tfs_close(f) != -1);

	return p;
}

void* thread02(void *p){
	int f = tfs_open(path1, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents2, sizeof(file_contents2)) ==
           sizeof(file_contents2));

    assert(tfs_close(f) != -1);

	return p;
}


void* thread03(void *p){
	int f = tfs_open(path1, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents1)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(file_contents1) || tfs_read(f, buffer, sizeof(buffer)) == sizeof(file_contents2));
	printf("this : %s\n", buffer);
    //assert(memcmp(buffer, file_contents2, sizeof(buffer)) == 0 || memcmp(buffer, file_contents1, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);

	return p;
}


void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents1)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

int main(){
    assert(tfs_init(NULL) != -1);
    int f = tfs_open(path1, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);

    assert_empty_file(path1); // sanity check

    pthread_t tid[3];
    
    for(int i = 0; i < 2000; i++){

        
        
        
        assert(pthread_create(&tid[0], NULL, thread01, NULL) == 0);
        assert(pthread_create(&tid[1], NULL, thread02, NULL) == 0);
        assert(pthread_create(&tid[2], NULL, thread03, NULL) == 0);



        pthread_join(tid[0], NULL);
        pthread_join(tid[1], NULL);
        pthread_join(tid[2], NULL);

        
    }
    assert(tfs_destroy() != -1);
    printf("Successful test.\n");
	
	return 0;
}
