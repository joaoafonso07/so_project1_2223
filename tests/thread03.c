#include "fs/operations.h"
#include "fs/state.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

uint8_t const file_contents1[] = "BBB! ";
char const path1[] = "/f1";
char const link1[] = "/l1";

void* thread01(void *p){
	int f1 = tfs_open(path1, TFS_O_CREAT);
    assert(f1 != -1);

    assert(tfs_write(f1, file_contents1, sizeof(file_contents1)) == sizeof(file_contents1));

    assert(tfs_close(f1) != -1);

	return p;
}

void* thread02(void *p){
    sleep(1);
	tfs_link(path1, link1);
    
	return p;
}


void* thread03(void *p){
	sleep(2);
    for(int i = 0; i < 2; i++){
        tfs_unlink(path1);
    }
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

    int f1 = tfs_open(path1, TFS_O_CREAT);
    assert(f1 != -1);

    assert(tfs_close(f1) != -1);

    assert_empty_file(path1); // sanity check



    pthread_t tid[3];

   
        
    assert(pthread_create(&tid[0], NULL, thread01, NULL) == 0);
    assert(pthread_create(&tid[1], NULL, thread02, NULL) == 0);
    assert(pthread_create(&tid[2], NULL, thread03, NULL) == 0);
    


    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);
    
        

    assert(tfs_destroy() != -1);
    printf("Successful test if no ThreadSanitizer warnings.\n");
	
	return 0;
}