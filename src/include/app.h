#ifndef _APP_H_
#define _APP_H_

#define _POSIX_SOURCE
#define _BSD_SOURCE 
#define _XOPEN_SOURCE 501

#include "defs.h"
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


void redirect_fd(int oldfd, int newfd);
void send_file(int fd, char ** src);
void write_to_shm(int fd, sem_t * addr, hash_info * hash_data, int curr_files_shm); 
int is_regular_file(const char *path);


#endif