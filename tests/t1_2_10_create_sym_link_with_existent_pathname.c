#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

char const path1[] = "/f1";
char const path2[] = "/l1";



int main(){

    // init TécnicoFS
    assert(tfs_init(NULL) != -1);
    // create file with content
    {
        int f1 = tfs_open(path1, TFS_O_CREAT);
        assert(tfs_close(f1) != -1);
		int f2 = tfs_open(path2, TFS_O_CREAT);
        assert(tfs_close(f2) != -1);
    }

	//create hard link on file
	assert(tfs_sym_link(path1, path2) == -1);


	printf("Successful test.\n");

	return 0;
}
