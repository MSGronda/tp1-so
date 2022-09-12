#ifndef _DEFS_H_
#define _DEFS_H_


#define MD5_SIZE 32
#define SHM_SIZE 16384

#define READ 0 
#define WRITE 1


typedef struct hash_info {
	int pid;
	char hash[MD5_SIZE + 1];
	char file_name[256];
	int files_left;
} hash_info;

#endif