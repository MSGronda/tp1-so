#include "./../include/resource_manager.h"


void write_to_shm(int fd, void * data, size_t length, int pos) 
{
	if(pwrite(fd, data, length, pos * length) == -1){
		perror("Writing to shm");
		exit(ERROR_WRITING_PIPE);
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

	if((shm_data->mmap_addr = mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, shm_data->fd, 0)) == MAP_FAILED) {
		perror("Mapping shared memory");
		exit(ERROR_MAPPING_SHM);
	}
}


void open_shm(shm_info * shm_data) 
{
	if((shm_data->fd = shm_open(shm_data->name, O_RDONLY, S_IRUSR)) == -1) {
		perror("Opening shared memory");
		exit(ERROR_CREATING_SHM);
	}

	if((shm_data->mmap_addr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_data->fd, 0)) == MAP_FAILED) {
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
