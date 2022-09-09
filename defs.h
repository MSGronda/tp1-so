/***************************************************
  Defs.h
****************************************************/

#ifndef _defs_
#define _defs_

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
#define ERROR_UNMAPPING_SHM 15
#define ERROR_UNLINKING_SHM 16
#define ERROR_CLOSING_SHM 17
#define ERROR_CREATING_SHM 18
#define ERROR_TRUNCATE_SHM 19
#define ERROR_MAPPING_SHM 20
#define ERROR_CREATING_SEM 21
#define ERROR_CLOSING_SEM 22


// Constants
#define MD5_SIZE 32
#define MAX_NAME_LENGTH 100

#define NO_FILES_FOUND 2


// Structs
typedef struct hash_info{
	int pid;
	char hash[MD5_SIZE + 1];
	char file_name[MAX_NAME_LENGTH];
	int files_left;
}hash_info;


/* MACROS */

// If function returns an error, execution is ended
#define ERROR_CHECK(func, error_value, error_msg, exit_value) 	if( (func) == (error_value) ){ 	\
																																	perror((error_msg)); 		\
																																	exit((exit_value));			\
																																}
// If either of the two functios returns an error, execution is ended
#define D_ERROR_CHECK(func1, func2, error_value, error_msg, exit_value) 	if( ((func1) == (error_value)) || ((func2) == (error_value))){ 	\
																																							perror((error_msg)); 		\
																																							exit((exit_value));			\
																																						}
// Stores return value and checks if it is an error, if so, execution is ended
#define ERROR_CHECK_KEEP(func, value, error_value, error_msg, exit_value) if(((value) = (func)) == error_value){ \
																																						perror(error_msg); \
																																						exit(exit_value); \
																																					}

#endif