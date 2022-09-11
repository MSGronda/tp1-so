#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_


/* STRUCTS */
typedef struct shm_info {
	char * name;
	int fd;
	void * mmap_addr;
} shm_info;

typedef struct sem_info {
	char * name;
	sem_t * addr;
} sem_info;

/* PROTOTYPES */
void create_semaphore(sem_info * sem_data);

void create_shm(shm_info * shm_data);

void create_pipe(int * fd[2]);

void close_fd(int fd);

int create_slave();

#endif