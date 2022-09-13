#ifndef _RESOURCE_MANAGER_H_
#define _RESOURCE_MANAGER_H_


#define _BSD_SOURCE
#define _XOPEN_SOURCE 501

#include "defs.h"
#include "errors.h"

#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <unistd.h>


typedef struct shm_info {
	char * name;
	int fd;
	void * mmap_addr;
} shm_info;

typedef struct sem_info {
	char * name;
	sem_t * addr;
} sem_info;
/*
 * << send_to_file >>
 * ----------------------------------------------------------------------
 * Description: Writes the * src recieved on the fd provided.
 *              Checks for possible error during write().
 * ----------------------------------------------------------------------
 * Receives: 
 *      [fd] = file descriptor to write
 *      [data] = Data to be written
 *      [length] = Size of data to be written
 * Returns:--
 */

void send_to_fd(int fd, void * data, size_t length);


/*
 * << write_to_shm >>
 * ----------------------------------------------------------------------
 * Description: Writes a [hash_data] struct the shm at a given offset. 
 *              Signals the semaphore recieved after writing.
 * ----------------------------------------------------------------------
 * Receives: 
 *      [fd] = file descriptor of the shm
 *      [data] = data to be written 
 *      [length] = size of data to be written
 *      [pos] = position to be written
 * Returns:--
 */
void write_to_shm(int fd, void * data, size_t length, int pos);

/*
 * << create_semaphore >>
 * ----------------------------------------------------------------------
 * Description: Creates semaphore with information from [sem_data]. 
 * Semaphore has RDWR permissions.
 * Exits in case of error? NO
 * ----------------------------------------------------------------------
 * Receives: 
 *      [sem_data] = Struct to save info relating semaphore
 * Returns: 
 * 		(void *) address of new semaphore
 * 		
 */
void * create_semaphore(sem_info * sem_data);

/*
 * << open_semaphore >>
 * ----------------------------------------------------------------------
 * Description: Opens semaphore with information from [sem_data]. That
 * semaphore needs to have been created previously. Semaphore has READ 
 * permissions. 
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [sem_data] = Struct to save info relating semaphore
 * Returns: --
 */
void open_semaphore(sem_info * sem_data);

/*
 * << close_semaphore >>
 * ----------------------------------------------------------------------
 * Description: Closes semaphore with existing information from [sem_data]
 * 
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [sem_data] = Struct with data relating to semaphore
 * Returns: --
 */
void close_semaphore(sem_info * sem_data);

/*
 * << unlink_semaphore >>
 * ----------------------------------------------------------------------
 * Description: Unlinks semaphore with existing information from [sem_data]
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [sem_name] = Name of semaphore to be unlinked
 * Returns: --
 */
void unlink_semaphore(char * sem_name);

/*
 * << create_shm >>
 * ----------------------------------------------------------------------
 * Description: Creates a zone of memory which is shared between different
 * processes. Shm has RDWR permissions. Shm has SHM_SIZE (->defs.h) size.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [shm_data] = Struct to save info relating to the shm space
 * Returns: --
 */
void create_shm(shm_info * shm_data);

/*
 * << open_shm >>
 * ----------------------------------------------------------------------
 * Description: Opens a zone of memory which is shared between different
 * processes. That zone needs to have been created previously. 
 * Shm has READ permissions. Shm has SHM_SIZE (->defs.h) size.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [shm_data] = Struct to save info relating to the shm space
 * Returns: --
 */
void open_shm(shm_info * shm_data); 

/*
 * << close_shm >>
 * ----------------------------------------------------------------------
 * Description: Closes a zone of memory which is shared between different
 * processes.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [shm_data] = Struct saving info relating to the shm space
 * Returns: --
 */
void close_shm(shm_info * shm_data);

/*
 * << close_shm >>
 * ----------------------------------------------------------------------
 * Description: Unlinks a zone of memory which is shared between different
 * processes. Exits if there is an error.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [shm_data] = Struct saving info relating to the shm space
 * Returns: --
 */
void unlink_shm(char * shm_name);

/*
 * << create_pipe >>
 * ----------------------------------------------------------------------
 * Description: Creates pipe. Exits if there is an error.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [fd] = Array used to return two file descriptors, fd[0] is read end
 *				and fd[1] is write end
 * Returns: --
 */
void create_pipe(int fd[2]);

/*
 * << close_fd >>
 * ----------------------------------------------------------------------
 * Description: Closes a fd.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [fd] = file descriptor to be closed
 * Returns: --
 */
void close_fd(int fd);

/*
 * << create_file >>
 * ----------------------------------------------------------------------
 * Description: Creates a file with [pathname] and with [mode] permissions.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [pathname] = Name of file to be created
 *		[mode] = refer to man 3 fopen
 * Returns: --
 */
FILE * create_file( char * pathname, char * mode);

/*
 * << close_file >>
 * ----------------------------------------------------------------------
 * Description: flushes the stream pointed to by [stream] and closes 
 * underlying fd.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: 
 *      [stream] = stream to be flushed
 * Returns: --
 */
void close_file(FILE * stream);

/*
 * << create_slave >>
 * ----------------------------------------------------------------------
 * Description: forks a process.
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: --
 * Returns: (int)
 *		0 <=> child process
 *		pid of child <=> parent process 
 */
int create_slave();

/*
 * << redirect_fd >>
 * ----------------------------------------------------------------------
 * Description: Redirects [oldfd] to [newfd].
 * Exits in case of error? YES
 * ----------------------------------------------------------------------
 * Receives: --
 * Returns: (int)
 *		0 <=> child process
 *		pid of child <=> parent process 
 */
void redirect_fd(int oldfd, int newfd);

#endif