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
	if(sem_close(sem_data->addr) == -1) {
		perror("Closing semaphore");
		exit(ERROR_CLOSING_SEM);
	}
}

void unlink_sem(char * sem_name)
{
	if(sem_unlink(sem_name) == -1) {
		perror("Unlinking semaphore");
		exit(ERROR_UNLINKING_SEM);
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

void unlink_shm(char * shm_name)
{
	if(shm_unlink(shm_name) == -1) {
		perror("Unlinking shared memory");
		exit(ERROR_UNLINKING_SHM);
	}
}


// TODO: No se si es con * o sin
void create_pipe(int fd[2]) 
{
	if(pipe(fd) == -1) {
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


void create_file(const char * pathname, const char * mode, FILE * out)
{
	if((out = fopen("respuesta.txt", "w")) == NULL) {
		perror("Creating output file");
		exit(ERROR_CREATING_FILE);
	}
}

void close_file(FILE * stream)
{
	if(fclose(stream) == EOF) {
		perror("Closing file");
		exit(ERROR_CLOSING_FILE);
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

