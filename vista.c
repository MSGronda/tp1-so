// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include "vista.h"
#include "defs.h"

#define MAX_NAME_LENGTH 100

void open_shm_sem(shared_resource_info * resources){

	// Map shared memory
	ERROR_CHECK_KEEP(shm_open(resources->shared_memory_name, O_RDONLY, S_IRUSR), resources->shm_fd, -1, "Creating shared memory", ERROR_CREATING_SHM)
	ERROR_CHECK_KEEP(mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, resources->shm_fd, 0), resources->mmap_addr, MAP_FAILED, "Mapping shared memory", ERROR_MAPPING_SHM)

	// Open semaphore for reading shared memory
	ERROR_CHECK_KEEP(sem_open(resources->read_sem_name,  O_RDONLY, S_IRUSR, 0), resources->read_sem, SEM_FAILED, "Creating semaphore", ERROR_CREATING_SEM)

	// Open semaphore for closing shared memory
	ERROR_CHECK_KEEP(sem_open(resources->close_sem_name,  O_RDONLY, S_IRUSR, 0), resources->close_sem, SEM_FAILED, "Creating semaphore", ERROR_CREATING_SEM)
}


void free_resources(shared_resource_info * resources){
	// Unmapping and closing of shared memory
	ERROR_CHECK(munmap(resources->mmap_addr, SHM_SIZE), -1, "Unmapping shared memory", ERROR_UNMAPPING_SHM)
	ERROR_CHECK(close(resources->shm_fd), -1, "Closing shared memory", ERROR_CLOSING_SHM)

	// Closing semaphore
	ERROR_CHECK(sem_close(resources->read_sem), -1, "Closing semaphore", ERROR_CLOSING_SEM)
	ERROR_CHECK(sem_close(resources->close_sem), -1, "Closing semaphore", ERROR_CLOSING_SEM)
}

int main(int argc, char * argv[]){

	/* --- Creating local variables --- */

	char * shared_memory_name, * read_sem_name, * close_sem_name;
	shared_resource_info resources;
	hash_info hash;

	/* --- Shared memory parameters --- */

	// If arguments are sent via arguments
	if(argc >= 4){
		shared_memory_name = argv[1];
		read_sem_name = argv[2];
		close_sem_name = argv[3];
	}
	// If arguments are sent via stdin
	else{
		
		//TODO: free en este caso!!!!
		//TODO: chequeo de errores

		shared_memory_name = malloc(MAX_NAME_LENGTH);
		read_sem_name = malloc(MAX_NAME_LENGTH);
		close_sem_name = malloc(MAX_NAME_LENGTH);

		fgets(shared_memory_name, MAX_NAME_LENGTH, stdin);
		fgets(read_sem_name, MAX_NAME_LENGTH, stdin);
		fgets(close_sem_name, MAX_NAME_LENGTH, stdin);

		shared_memory_name[strlen(shared_memory_name) -1] = 0;
		read_sem_name[strlen(read_sem_name) -1] = 0;
		close_sem_name[strlen(close_sem_name) -1] = 0;
	}

	resources.shared_memory_name = shared_memory_name;
	resources.read_sem_name = read_sem_name;
	resources.close_sem_name = close_sem_name;

	open_shm_sem(&resources);

	// Signal to app that resources are being read so they should not be unlinked
	sem_wait(resources.close_sem);

	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	/* --- Recieving answer and broadcasting --- */

	printf("Calculating md5 hash...\n");

	for(int i=0, finished=0; !finished; i++){
		sem_wait(resources.read_sem);

		pread(resources.shm_fd, &hash, sizeof(hash_info), i * sizeof(hash_info));
		printf("\nFile: %s Md5: %s Pid: %d\n",hash.file_name, hash.hash, hash.pid);

		if(hash.files_left <= 1)
			finished = 1;		
	}

	printf("\nFinished calculating md5!\n\n");


	/* --- Freeing resources --- */
	// Signal to app that all hashes have been read. They can now be unlinked.
	sem_post(resources.close_sem);
	free_resources(&resources);

	return 0;
}
