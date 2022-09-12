#ifndef _APP_H_
#define _APP_H_


#define _POSIX_SOURCE
#define _BSD_SOURCE 
#define _XOPEN_SOURCE 501

#include "defs.h"
#include "errors.h"
#include "resource_manager.h"
#include "slave.h"

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>


/*
 * << exit_handler >>
 * ----------------------------------------------------------------------
 * Description: Frees memory before exitting, this function is then passed
 * to on_exit(), that´s why it has [code] and [arr].
 * ----------------------------------------------------------------------
 * Receives: --
 * Returns:--
 */
void exit_handler(int code, void * arr);


/*
 * << send_file >>
 * ----------------------------------------------------------------------
 * Description: Writes the * src recieved on the fd provided.
 *              Checks for possible error during write().
 * ----------------------------------------------------------------------
 * Receives: 
 *      [fd] = file descriptor to write
 *      [src] = Pointer to the string to be written
 * Returns:--
 */
void send_file(int fd, char ** src);

/*
 * << write_to_shm >>
 * ----------------------------------------------------------------------
 * Description: Writes a [hash_data] struct the shm at a given offset. 
 *              Signals the semaphore recieved after writing.
 * ----------------------------------------------------------------------
 * Receives: 
 *      [fd] = file descriptor of the shm
 *      [addr] = semaphore address
 *      [hash_data] = struct containing md5 hash info of a given file
 *      [curr_files_shm] = current file count for calculating offset
 * Returns:--
 */
void write_to_shm(int fd, sem_t * addr, hash_info * hash_data, int curr_files_shm); 

/*
 * << is_regular_file >>
 * ----------------------------------------------------------------------
 * Description: Checks is a provided path is a regular file
 * ----------------------------------------------------------------------
 * Receives: 
 *      [path] = path to a given file
 * Returns:
 *      1 in case it is a regular file
 *      0 if not
 */
int is_regular_file(const char *path);


#endif