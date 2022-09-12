#include "./include/vista.h"

int main(int argc, char * argv[]) 
{
	char shm_name[NAME_MAX], sem_read_name[NAME_MAX], sem_close_name[NAME_MAX];
	shm_info shm_data;
	sem_info semaphore_read, semaphore_close;
	hash_info hash_data;

	if(argc >= 4) { // arguments are sent via function arguments
		strncpy(shm_name, argv[1], NAME_MAX);
		strncpy(sem_read_name, argv[2], NAME_MAX);
		strncpy(sem_close_name, argv[3], NAME_MAX);
	}
	else //arguments are sent via stdin
	{
		fgets(shm_name, NAME_MAX, stdin);
		fgets(sem_read_name, NAME_MAX, stdin);
		fgets(sem_close_name, NAME_MAX, stdin);

		shm_name[strlen(shm_name)-1] = 0;
		sem_read_name[strlen(sem_read_name)-1] = 0;
		sem_close_name[strlen(sem_close_name)-1] = 0;
	}

	shm_data.name = shm_name;
	semaphore_read.name = sem_read_name;
	semaphore_close.name = sem_close_name;

	open_shm(&shm_data);
	open_semaphore(&semaphore_read);
	open_semaphore(&semaphore_close);

	// Signal to app that resources are being read so they should not be unlinked
	sem_wait(semaphore_close.addr);

	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	/* --- Receiving answer and broadcasting --- */
	printf("Calculating md5 hash...\n");

	for(int i = 0, finished = 0; !finished; i++) {
		sem_wait(semaphore_read.addr);

		if(pread(shm_data.fd, &hash_data, sizeof(hash_info), i * sizeof(hash_info)) == -1){
			sem_post(semaphore_close.addr);		// Allow app to free resources
			perror("Reading shared memory");
			exit(ERROR_READING_SHM);
		}
		
		printf("\n -> File: %s | Md5: %s | Pid: %d\n",hash_data.file_name, hash_data.hash, hash_data.pid);

		if(hash_data.files_left <= 1)
			finished = 1;	
	}

	printf("\nFinished calculating md5!\n\n");

	/* --- Freeing resources --- */
	// Signal to app that all hashes have been read. They can now be unlinked.
	sem_post(semaphore_close.addr);
	close_shm(&shm_data);
	close_semaphore(&semaphore_read);
	close_semaphore(&semaphore_close);

	return 0;
}