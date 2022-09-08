#define _POSIX_SOURCE
#define _BSD_SOURCE 
#define _XOPEN_SOURCE 501


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

// Error codes
#define ERROR_CREATING_SLAVE_PIPES 1
#define ERROR_CREATING_SLAVE 2
#define ERROR_CLOSING_SLAVE_PIPES 3
#define ERROR_CREATING_SUBSLAVE_PIPES 4
#define ERROR_SETTING_SUBSLAVE_PIPES 5
#define ERROR_CREATING_SUBSLAVE 6
#define ERROR_SELECT_APP 7
#define ERROR_READ_SLAVE_PIPE 8
#define ERROR_CLOSING_APP_PIPES 9
#define ERROR_SELECT_SLAVE 10
#define ERROR_SELECT_APP_TO_SLAVE 11
#define ERROR_SELECT_SUBSLAVE 12
#define ERROR_READ_SUBSLAVE_PIPE 13
#define ERROR_CLOSING_FINAL_PIPES 14

#define MD5_SIZE 32
#define MAX_NAME_LENGTH 100

typedef struct hash_info{
	int pid;
	char hash[MD5_SIZE + 1];
	char file_name[MAX_NAME_LENGTH];	
}hash_info;

