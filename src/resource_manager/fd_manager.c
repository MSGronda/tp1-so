#include "./../include/resource_manager.h"

void create_pipe(int fd[2]) 
{
	if(pipe(fd) == -1) {
		perror("Creating pipes");
		exit(ERROR_CREATING_PIPES);
	}
}

void close_fd(int fd) 
{
	if(close(fd) == -1) {
		perror("Closing fd");
		exit(ERROR_CLOSING_FD);
	}
}

void redirect_fd(int oldfd, int newfd)
{
	if(dup2(oldfd, newfd) == -1) {
		perror("Dup2");
		exit(ERROR_DUP2);
	} 
}
