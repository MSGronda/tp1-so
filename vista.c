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

#include <readline/readline.h>
#include <readline/history.h>

#include "defs.h"


int main(int argc, char * argv[]){

	char * shared_memory_name, * semaphore_name;

	if(argc >= 3){
		shared_memory_name = argv[1];
		semaphore_name = argv[2];
	}
	else{
		shared_memory_name = malloc(100);
		semaphore_name = malloc(100);

		fgets(shared_memory_name, 100, stdin);
		fgets(semaphore_name, 100, stdin);
	}

	shared_memory_name[strlen(shared_memory_name) -1] = 0;
	semaphore_name[strlen(semaphore_name) -1] = 0;


	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	// Map shared memory
	int shm_fd;
	ERROR_CHECK_KEEP(shm_open(shared_memory_name, O_RDONLY, S_IRUSR), shm_fd, -1, "Creating shared memory", ERROR_CREATING_SHM)
	void * mmap_addr;
	ERROR_CHECK_KEEP(mmap(NULL, 3000, PROT_READ, MAP_SHARED, shm_fd, 0),mmap_addr, MAP_FAILED, "Mapping shared memory", ERROR_MAPPING_SHM)

	// Open semaphore for shared memory
	sem_t * sem_smh;
	ERROR_CHECK_KEEP(sem_open(semaphore_name,  O_RDONLY, S_IRUSR, 0), sem_smh, SEM_FAILED, "Creating semaphore", ERROR_CREATING_SEM)


	hash_info hash;
	for(int i=0, finished=0; !finished; i++){
		sem_wait(sem_smh);
		
		pread(shm_fd, &hash, sizeof(hash_info), i * sizeof(hash_info));

		if(hash.files_left <= 1){
			finished = 1;
		}

		printf("File: %s Md5: %s Pid: %d\n",hash.file_name, hash.hash, hash.pid);
	}
}