#include "resource_manager.h"

void create_semaphore(sem_info * sem_data)
{
	if((sem_data->addr = sem_open(sem_data->name,  O_CREAT|O_RDWR, S_IRUSR|S_IWUSR, 0)) == -1) {
		perror("Creating semaphore");
		exit(ERROR_CREATING_SEMAPHORE);
	}
}

void close_semaphore(sem_info * sem_data)
{
	if(sem_close(resources->read_sem) == -1) {
		perror("Closing semaphore");
		exit(ERROR_CLOSING_SEM);
	}
}

void create_shm(shm_info * shm_data) 
{
	if((shm_data->fd = shm_open(shm_data->name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR)) == -1) {
		perror("Creating shared memory");
		exit(ERROR_CREATING_SHM);
	}
	
	if(ftruncate(shm_data->fd, SHM_SIZE) == -1) {
		perror("Truncating shared memory");
		exit(ERROR_TRUNCATE_SHM);
	}

	if((shm_data->mmap_addr = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_data->fd, 0)) == -1) {
		perror("Mapping shared memory");
		exit(ERROR_MAPPING_SHM);
	}
}

void close_shm(shm_info * shm_data)
{
	if(munmap(shm_data->mmap_addr, SHM_SIZE) == -1) {
		perror("Unmapping shared memory");
		exit(ERROR_UNMAPPING_SHM);
	}

	if(close(shm_data->fd) == -1) {
		perror("Closing shared memory");
		exit(ERROR_CLOSING_SHM);
	}
}

// TODO: No se si es con * o sin
void create_pipe(int * fd[2]) 
{
	if(pipe(&fd) == -1) {
		perror("Creating pipes");
		exit(ERROR_CREATING_PIPES);
	}
}

void close_fd(int fd) 
{
	if(close(fd) == -1) {
		perror("Closing fd");
		exit(ERROR_CLOSING_FD);
	}
}

int create_slave() 
{
	int out = 0;
	if((out = fork()) == -1) {
		perror("Creating slaves");
		exit(ERROR_CREATING_SLAVE);
	}

	return out;
}
