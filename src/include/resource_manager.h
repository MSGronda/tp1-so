#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_

#define _BSD_SOURCE

#include "defs.h"
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <unistd.h>


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

void open_semaphore(sem_info * sem_data);

void close_semaphore(sem_info * sem_data);

void unlink_semaphore(char * sem_name);


void create_shm(shm_info * shm_data);

void open_shm(shm_info * shm_data); 

void close_shm(shm_info * shm_data);

void unlink_shm(char * shm_name);


void create_pipe(int fd[2]);

void close_fd(int fd);


void create_file(const char * pathname, const char * mode, FILE * out);

void close_file(FILE * stream);


int create_slave();

#endif