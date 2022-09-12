#ifndef _ERRORS_H_
#define _ERRORS_H_

/* --- ERROR CODES --- */
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
#define ERROR_WRITING_PIPE 23
#define NO_FILES_FOUND 24
#define ERROR_CREATING_FILE 25
#define ERROR_CLOSING_FILE 26
#define ERROR_CREATING_PIPES 27
#define ERROR_UNLINKING_SEM 28
#define ERROR_CLOSING_FD 29
#define ERROR_DUP2 30
#define NO_ARGS 31
#define ERROR_READING_SHM 32
#define ERROR_WRITING_SHM 33


#define CHECK_ERROR(left_val, right_val, msg, exit_val)	if( (left_val) == (right_val) ) { 	\
															perror(msg);					\
															exit(exit_val);					\
														}

#endif