#include "./../include/resource_manager.h"

FILE * create_file(char * pathname,  char * mode)
{
	FILE * out;
	if((out = fopen("respuesta.txt", "w")) == NULL) {
		perror("Creating output file");
		exit(ERROR_CREATING_FILE);
	}
	return out;
}

void close_file(FILE * stream)
{
	if(fclose(stream) == EOF) {
		perror("Closing file");
		exit(ERROR_CLOSING_FILE);
	}
}


void send_file(int fd, void * data, size_t length)
{
	if(write(fd, data, length) == -1) {
		perror("Writing to slave");
		exit(ERROR_WRITING_PIPE);
	}
}
