#define _POSIX_SOURCE
#define _BSD_SOURCE 
#define _XOPEN_SOURCE 501


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>
#include <sys/mman.h>

#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>

#include "defs.h"

void open_shm_sem(shared_resource_info * resources){

	// Map shared memory
	ERROR_CHECK_KEEP(shm_open(resources->shared_memory_name, O_RDONLY, S_IRUSR), resources->shm_fd, -1, "Creating shared memory", ERROR_CREATING_SHM)
	ERROR_CHECK_KEEP(mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, resources->shm_fd, 0), resources->mmap_addr, MAP_FAILED, "Mapping shared memory", ERROR_MAPPING_SHM)

	// Open semaphore for shared memory
	ERROR_CHECK_KEEP(sem_open(resources->semaphore_name,  O_RDONLY, S_IRUSR, 0), resources->sem_smh, SEM_FAILED, "Creating semaphore", ERROR_CREATING_SEM)
}


void free_resources(shared_resource_info * resources){
	// Unmapping and closing of shared memory
	ERROR_CHECK(munmap(resources->mmap_addr, 3000), -1, "Unmapping shared memory", ERROR_UNMAPPING_SHM)
	
	ERROR_CHECK(close(resources->shm_fd), -1, "Closing shared memory", ERROR_CLOSING_SHM)

	// Closing semaphore
	ERROR_CHECK(sem_close(resources->sem_smh), -1, "Closing semaphore", ERROR_CLOSING_SEM)

}

int main(int argc, char * argv[]){

	/* --- Creating local variables --- */

	char * shared_memory_name, * semaphore_name;
	shared_resource_info resources;
	hash_info hash;

	/* --- Shared memory parameters --- */

	if(argc >= 3){
		shared_memory_name = argv[1];
		semaphore_name = argv[2];
	}
	else{
		shared_memory_name = malloc(100);
		semaphore_name = malloc(100);

		fgets(shared_memory_name, 100, stdin);
		fgets(semaphore_name, 100, stdin);

		shared_memory_name[strlen(shared_memory_name) -1] = 0;
		semaphore_name[strlen(semaphore_name) -1] = 0;
	}
	resources.shared_memory_name = shared_memory_name;
	resources.semaphore_name = semaphore_name;

	open_shm_sem(&resources);

	/* --- Printing info --- */

	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	printf("Calculating md5 hash...\n");


	/* --- Recieving answer and broadcasting --- */

	for(int i=0, finished=0; !finished; i++){
		sem_wait(resources.sem_smh);

		pread(resources.shm_fd, &hash, sizeof(hash_info), i * sizeof(hash_info));
		printf("\nFile: %s Md5: %s Pid: %d\n",hash.file_name, hash.hash, hash.pid);

		if(hash.files_left <= 1)
			finished = 1;		
	}

	printf("\nFinished calculating md5!\n\n");


	/* --- Freeing resources --- */
	free_resources(&resources);

	return 0;
}