#include "./../include/resource_manager.h"

int create_slave() 
{
	int out = 0;
	if((out = fork()) == -1) {
		perror("Creating slaves");
		exit(ERROR_CREATING_SLAVE);
	}

	return out;
}
