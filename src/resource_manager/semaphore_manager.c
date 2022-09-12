#include "./../include/resource_manager.h"

void * create_semaphore(sem_info * sem_data) {
	return (sem_data->addr = sem_open(sem_data->name,  O_CREAT|O_RDWR, S_IRUSR|S_IWUSR, 0));
}

void open_semaphore(sem_info * sem_data)
{
	if((sem_data->addr = sem_open(sem_data->name, O_RDONLY, S_IRUSR, 0)) == SEM_FAILED) {
		perror("Creating semaphore");
		exit(ERROR_CREATING_SEM);
	}
}

void close_semaphore(sem_info * sem_data)
{
	if(sem_close(sem_data->addr) == -1) {
		perror("Closing semaphore");
		exit(ERROR_CLOSING_SEM);
	}
}

void unlink_semaphore(char * sem_name)
{
	if(sem_unlink(sem_name) == -1) {
		perror("Unlinking semaphore");
		exit(ERROR_UNLINKING_SEM);
	}
}
