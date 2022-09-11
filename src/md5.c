#include "defs.h"
#include "resource_manager.h"

/* CONSTANTS */
#define FILES_PER_SLAVE 2

#define READ 0 
#define WRITE 1

#define NORMAL 0
#define BACKUP 1

#define SHM_NAME "md5_shm"
#define SEM_READ_NAME "md5_read_sem"
#define SEM_CLOSE_NAME "md5_shm_sem"


typedef struct slave_info {
	int app_to_slave[2];
	int slave_to_app[2];
	int pid;

	char * prev_file_name;
} slave_info;

// Contains information of pipes from subslave to slave
typedef struct subslave_info{
	int fd_pipe[2];
	fd_set set[2];
} subslave_info;


int main(int argc, char * argv[])
{
	if(argc <= 1) return NO_FILES_FOUND;

	/* --- Creation of local variables and necessary resources --- */
	int num_files = argc - 1;
	int num_slaves = ceil((double) num_files / FILES_PER_SLAVE); 	// TODO: ARREGLAR ESTO

	//	Create shared memory and semaphore
	shm_info shm_data;
	sem_info semaphore_read, semaphore_close;

	shm_data.name = SHM_NAME;
	semaphore_read.name = SEM_READ_NAME;
	semaphore_close.name = SEM_CLOSE_NAME;

	create_shm(&shm_data);
	create_semaphore(&semaphore_read);
	create_semaphore(&semaphore_close);
	sem_post(semaphore_close.addr);


	// Creating pipes for slaves and adding them to the select set
	slave_info slaves[num_slaves];
	fd_set fd_read, fd_backup_read;

	FD_ZERO(fd_read);

	for(int i=0; i < num_slaves; i++){
		create_pipe(&slaves[i].app_to_slave);
		create_pipe(&slaves[i].slave_to_app);
		
		FD_SET(slaves[i].slave_to_app[READ], fd_read);
	}
	backup_read_fd = read_fd;

	// Create file for output
	FILE * output;
	create_file("respueta.txt", "w", output);

	/* --- Broadcast for VISTA process --- */
	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);
	
	printf("%s\n", SHM_NAME); 			// Broadcast shared memory address
	printf("%s\n", SEM_READ_NAME); 		// Broadcast read shared memory sempaphore
	printf("%s\n", SEM_CLOSE_NAME);		// Broadcast close shared memory sempaphore

	sleep(2);

	/* --- Creating slaves --- */
	int curr_slave, curr_id = 1;
	for(curr_slave = 0; curr_slave < num_slaves && curr_id != 0 ; curr_slave++) {
		curr_id = create_slave();
		
		slaves[curr_slave].pid = curr_id;
	}

	switch(curr_id) {
		/* --- SLAVE process is running --- */
		case 0:
			slave(slaves[curr_slave - 1].app_to_slave, slaves[curr_slave - 1].slave_to_app);
			break;

    	/* --- APP process is running --- */
		default:
			/* --- Creation of local variables --- */
			char ans[MD5_SIZE + 1]= {0};
			int curr_files_sent = 1, curr_files_read = 0;	// Ignore first file bc it is the executable's name
			hash_info hash_data;

			/* --- Close useless ends of pipes --- */
			for(int i = 0; i < num_slaves; i++) {
				close_fd(slaves[i].app_to_slave[READ]);
				close_fd(slaves[i].slave_to_app[WRITE]);
	        }

	        /* --- Initial distribution of files to slaves --- */
			for(int i = 0; curr_files_sent < num_slaves; i++) {
				send_file(slaves[i].app_to_slave[WRITE], argv[curr_files_sent]);
				slaves[i].prev_file_name = argv[curr_files_sent];

				curr_files_sent++;
			}

			/* --- File data is received from subslave process --- */
			while(curr_files_read < num_files) {
				if(select(FD_SETSIZE, &read_fd, NULL, NULL, NULL) == -1) {
					perror("Select in app");
					exit(ERROR_SELECT_APP);
				}

				for(int i = 0; i < num_slaves && curr_files_read < num_files; i++) {
					// Select notified that this pipe has something in it
					if(FD_ISSET(slaves[i].slave_to_app[READ], &read_fd)) {
						
						if(read(slaves[i].slave_to_app[READ], ans, MD5_SIZE*sizeof(char)) == -1) {
							perror("Read in app");
							exit(ERROR_READ_SLAVE_PIPE);
						}

						// Complete hash_info fields
						hash_data.pid = pid;
						strcpy(hash_data.hash, ans);
						strcpy(hash_data.file_name, prev_file_name);  
						hash_data.files_left = num_files - curr_files_shm;

						write_to_shm(shm_data.fd, semaphore_read.addr, &hash_data, curr_files_read);
						curr_files_read++;

						// Write hash to file
						fprintf(output, "\nFile: %s Md5: %s Pid: %d\n", hash_data.file_name, hash_data.hash, hash_data.pid);

						if(curr_files_sent <= num_files) {
							send_file(slaves[i].app_to_slave[WRITE], argv[curr_files_sent]);
							slaves[i].prev_file_name = argv[curr_files_sent];

							curr_files_sent++;
						}
					}
				}
				fd_read = fd_backup_read;
			}

			/* --- Free Resources --- */
			for(int i = 0; i < num_slaves; i++) {
				close_fd(slaves[i].app_to_slave[WRITE]);
				close_fd(slaves[i].slave_to_app[READ]);
				
				kill(slaves[i].pid, SIGKILL);
			}

			close_shm(&shm_data);
			close_semaphore(&semaphore_read);
			close_file(output);

			// Wait until vista stops reading shared memory. If vista doesnt exist, continue.
			sem_wait(semaphore_close.addr);

			close_semaphore(&semaphore_close);

			unlink_shm(shm_data.name);
			unlink_sem(sem_data.name);
			unlink_sem(sem_data.name);
			break;
	}

	return 0;
}


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
	FD_ZERO(app_to_slave_set[NORMAL]);
	FD_SET(app_to_slave[READ], app_to_slave_set[NORMAL]);
	app_to_slave_set[BACKUP] = app_to_slave_set[NORMAL];

	create_pipe(subslave.fd_pipe);
	redirect_fd(subslave.fd_pipe[READ], 0);
	redirect_fd(subslave.fd_pipe[WRITE], 1);

	FD_ZERO(subslave.set[NORMAL]);
	FD_SET(0, subslave.set[NORMAL]);
	subslave.set[BACKUP] = subslave.set[NORMAL];

	while(!finished) {
		if(select(FD_SETSIZE, &app_to_slave_set[NORMAL], NULL, NULL, NULL) == -1) {
			perror("Select in slave");
			exit(ERROR_SELECT_APP);
		}

		app_to_slave_set[NORMAL] = app_to_slave_set[BACKUP];		// Backup to previous state
		if(read(slaves[i].slave_to_app[READ], &file_name, sizeof(char *)) == -1) {
			perror("Reading answer from subslave.");
			exit(ERROR_SELECT_APP_TO_SLAVE);
		}

		int id = create_slave();
		switch(id) {
			/* SUBSLAVE: Whose job is to execute md5sum */
			case 0:
				args[1] = file_name;	
				execvp("md5sum", args);
				break;

			/* SLAVE */
			default:
				if(select(FD_SETSIZE, &(subslave.set[NORMAL]), NULL, NULL, NULL) == -1) {
					perror("Select in slave for reading pipe from app.");
					exit(ERROR_SELECT_SUBSLAVE);
				}
				subslave.set[NORMAL] = subslave.set[BACKUP];		// Backup to previous state

				if(read(0, output, sizeof(char)) == -1) {
					perror("Reading answer from subslave.");
					exit(ERROR_READ_SUBSLAVE_PIPE);
				}

				// Flush stdin
				int dump;
				while ((dump = getchar()) != '\n' && dump != EOF);

				wait(NULL); // Wait for subslave

				// Write md5 hash to pipe that goes to app
				if(write(slave_to_app[WRITE], output, MD5_SIZE*sizeof(char)) == -1) {
					perror("Writing hash to app");
					exit(ERROR_WRITING_PIPE);
				}
		}
	}
	return 0;
}


void redirect_fd(int oldfd, int newfd)
{
	if(dup2(oldfd, newfd) == -1) {
		perror("Dup2");
		exit(ERROR_DUP2);
	} 
}


// No se que onda los *
void send_file(int fd, const char * src)
{
	if(write(fd, &src, sizeof(char *)) == -1) {
		perror("Writing to slave");
		exit(ERROR_WRITING_PIPE);
	}
}

// No se que onda los *
void write_to_shm(int fd, sem_t * addr, hash_info * hash_data, int curr_files_shm) 
{
	//TODO: chequeo de errores

	pwrite(shm->fd, hash_data, sizeof(hash_data), curr_files_shm * sizeof(hash_info));

	sem_post(addr);
}
