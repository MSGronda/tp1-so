#define _POSIX_SOURCE
#define _BSD_SOURCE 
#define _XOPEN_SOURCE 501


#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>


#define SHARED_MEMORY_NAME "md5_shm"
#define SEMAPHORE_NAME "md5_sem"

#define MD5_SIZE 32
#define MAX_NAME_LENGTH 100

typedef struct hash_info{
	int pid;
	char hash[MD5_SIZE + 1];
	char file_name[MAX_NAME_LENGTH];
	int files_left;	
}hash_info;

int main(){

	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	int fd = shm_open(SHARED_MEMORY_NAME, O_RDONLY, S_IRUSR);
	mmap(NULL, 3000, PROT_READ, MAP_SHARED, fd, 0);


	sem_t * sem_smh = sem_open(SEMAPHORE_NAME,  O_RDONLY, S_IRUSR);
	hash_info hash;

	for(int i=0, finished=0; !finished; i++){
		sem_wait(sem_smh);
		pread(fd, &hash, sizeof(hash_info), i * sizeof(hash_info));

		if(hash.files_left <= 1){
			finished = 1;
		}

		printf("File: %s Md5: %s Pid: %d\n",hash.file_name, hash.hash, hash.pid);
	}
;
}