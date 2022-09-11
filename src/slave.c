#include "./include/slave.h"

// Contains information of pipes from subslave to slave
typedef struct subslave_info{
	int fd_pipe[2];
	fd_set set[2];
} subslave_info;

#define NORMAL 0
#define BACKUP 1


int slave(int * app_to_slave, int * slave_to_app)
{
	/* --- Creating local variables --- */
	// Slave to app and subslave to slave
	subslave_info subslave;
	fd_set app_to_slave_set[2];

	char * args[] = { "md5sum", NULL, NULL };	// 2nd arg will be path received from APP
	int finished = 0;					
	char * file_name;
	char output[MD5_SIZE + 1]= {0};		// calculated hash for file 

	/* --- Closing useless pipes and creating new ones --- */
	close_fd(app_to_slave[WRITE]);
	close_fd(slave_to_app[READ]);
	
	// TODO: cerrar todos los fd que no usamos

	// Read end of pipe between app and slave
	FD_ZERO(&(app_to_slave_set[NORMAL]));
	FD_SET(app_to_slave[READ], &(app_to_slave_set[NORMAL]));
	app_to_slave_set[BACKUP] = app_to_slave_set[NORMAL];

	create_pipe(subslave.fd_pipe);

	redirect_fd(subslave.fd_pipe[READ], 0);
	redirect_fd(subslave.fd_pipe[WRITE], 1);

	FD_ZERO(&(subslave.set[NORMAL]));
	FD_SET(0, &(subslave.set[NORMAL]));
	subslave.set[BACKUP] = subslave.set[NORMAL];

	while(!finished) {

		CHECK_ERROR(select(FD_SETSIZE, &(app_to_slave_set[NORMAL]), NULL, NULL, NULL), -1, "Select in slave", ERROR_SELECT_APP)
	
		app_to_slave_set[NORMAL] = app_to_slave_set[BACKUP];		// Backup to previous state
		CHECK_ERROR( read(app_to_slave[READ], &file_name, sizeof(char *)), -1, "Reading answer from subslave.", ERROR_SELECT_APP_TO_SLAVE)

		int id = create_slave();
		
		switch(id) {
			/* SUBSLAVE: Whose job is to execute md5sum */
			case 0:
				args[1] = file_name;	
				execvp("md5sum", args);
				break;

			/* SLAVE */
			default:
				CHECK_ERROR(select(FD_SETSIZE, &(subslave.set[NORMAL]), NULL, NULL, NULL), -1, "Select in slave for reading pipe from app.", ERROR_SELECT_SUBSLAVE)
				subslave.set[NORMAL] = subslave.set[BACKUP];		// Backup to previous state

				CHECK_ERROR(read(0, output, MD5_SIZE * sizeof(char)), -1, "Reading answer from subslave.",  ERROR_READ_SUBSLAVE_PIPE)

				// Flush stdin
				int dump;
				while ((dump = getchar()) != '\n' && dump != EOF);

				wait(NULL); // Wait for subslave

				// Write md5 hash to pipe that goes to app
				CHECK_ERROR(write(slave_to_app[WRITE], output, MD5_SIZE*sizeof(char)), -1, "Writing hash to app", ERROR_WRITING_PIPE)
		}
	}
	return 0;
}
