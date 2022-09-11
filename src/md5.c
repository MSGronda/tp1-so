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


int main(int argc, char * argv[])
{
	if(argc <= 1) return NO_FILES_FOUND;

	/* --- Creation of local variables and necesary resources --- */
	int num_files = argc - 1;
	int num_slaves = ceil((double) num_files / FILES_PER_SLAVE); // TODO: ARREGLAR ESTO

	//	Create shared memory and semaphore
	shm_info shm_data;
	sem_info sem_read, sem_close;

	shm_data.name = SHM_NAME;
	sem_read.name = SEM_READ_NAME;
	sem_close.name = SEM_CLOSE_NAME;

	create_shm(&shm_data);
	create_semaphore(&sem_read);
	create_semaphore(&sem_close);
	sem_post(sem_close.addr);


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
	if((output = fopen("respuesta.txt", "w")) == NULL) {
		perror("Creating output file");
		exit(ERROR_CREATING_FILE);
	}

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
			slave();
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

						write_to_shm(shm_data.fd, sem_read.addr, &hash_data, curr_files_read);
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
			break;
	}

	return 0;
}


void free_resources(slave_info * slaves, shared_resource_info * resources, int num_slaves, FILE * output) 
{

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
