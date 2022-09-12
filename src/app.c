#include "./include/app.h"

/* CONSTANTS */
#define FILES_PER_SLAVE 2

#define SHM_NAME "md5_shm"
#define SEM_READ_NAME "md5_read_sem"
#define SEM_CLOSE_NAME "md5_close_sem"

#define SLEEP_TIME 2

/* --- STRUCT --- */
typedef struct slave_info {
	int app_to_slave[2];
	int slave_to_app[2];
	int pid;

	char * prev_file_name;
} slave_info;


int main(int argc, char * argv[])
{
	char * files[argc];

	// We only keep arguments that are files, NOT directories
	int num_files=0;
	for(int i=1; i<argc; i++){
		if(is_regular_file(argv[i])){
			files[num_files] = argv[i];
			num_files++;
		}
	}

	if(argc <= 1 || num_files == 0) return NO_FILES_FOUND;

	/* --- Creation of local variables and necessary resources --- */
	int num_slaves = ceil((double) num_files / FILES_PER_SLAVE); 	// TODO: ARREGLAR ESTO

	// Creating pipes for slaves and adding them to the select set
	slave_info slaves[num_slaves];
	fd_set fd_read, fd_backup_read;

	FD_ZERO(&fd_read);

	for(int i=0; i < num_slaves; i++){
		create_pipe(slaves[i].app_to_slave);
		create_pipe(slaves[i].slave_to_app);
		
		FD_SET(slaves[i].slave_to_app[READ], &fd_read);
	}
	fd_backup_read = fd_read;

	// Create file for output
	FILE * output = create_file("respuesta.txt", "w");


	//	Create shared memory and semaphore
	shm_info shm_data;
	sem_info semaphore_read, semaphore_close;

	shm_data.name = SHM_NAME;
	semaphore_read.name = SEM_READ_NAME;
	semaphore_close.name = SEM_CLOSE_NAME;

	create_shm(&shm_data);
	void * resp ;
	if((resp = create_semaphore(&semaphore_read)) == SEM_FAILED){
		unlink_shm(shm_data.name);
		perror("Creating semaphore");
		exit(ERROR_CREATING_SEM);
	}

	if((resp = create_semaphore(&semaphore_close)) == SEM_FAILED){
		unlink_shm(shm_data.name);
		unlink_semaphore(semaphore_read.name);
		perror("Creating semaphore");
		exit(ERROR_CREATING_SEM);
	}

	
	sem_post(semaphore_close.addr);	


	/* --- Broadcast for VISTA process --- */
	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);
	
	printf("%s\n", SHM_NAME); 			// Broadcast shared memory address
	printf("%s\n", SEM_READ_NAME); 		// Broadcast read shared memory sempaphore
	printf("%s\n", SEM_CLOSE_NAME);		// Broadcast close shared memory sempaphore

	sleep(SLEEP_TIME);

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
		default:; 

			// App will exit freeing resources during normal execution and if error is encountered
			on_exit(exit_handler, NULL);

			/* --- Creation of local variables --- */
			char ans[MD5_SIZE + 1] = { 0 };
			int curr_files_sent = 0, curr_files_read = 0;	// Ignore first file bc it is the executable's name
			hash_info hash_data;

			/* --- Close useless ends of pipes --- */
			for(int i = 0; i < num_slaves; i++) {
				close_fd(slaves[i].app_to_slave[READ]);
				close_fd(slaves[i].slave_to_app[WRITE]);
	        }

	        /* --- Initial distribution of files to slaves --- */
			for(int i = 0; curr_files_sent < num_slaves; i++) {
				send_file(slaves[i].app_to_slave[WRITE], &(files[curr_files_sent]));
				slaves[i].prev_file_name = files[curr_files_sent];
				curr_files_sent++;
				
			}

			/* --- File data is received from subslave process --- */
			while(curr_files_read < num_files) {
				if(select(FD_SETSIZE, &fd_read, NULL, NULL, NULL) == -1) {
					perror("Select in app");
					exit(ERROR_SELECT_APP);
				}

				for(int i = 0; i < num_slaves && curr_files_read < num_files; i++) {
					// Select notified that this pipe has something in it
					if(FD_ISSET(slaves[i].slave_to_app[READ], &fd_read)) {
						
						if(read(slaves[i].slave_to_app[READ], ans, MD5_SIZE*sizeof(char)) == -1) {
							perror("Read in app");
							exit(ERROR_READ_SLAVE_PIPE);
						}

						// Complete hash_info fields
						hash_data.pid = slaves[i].pid;
						strcpy(hash_data.hash, ans);
						strcpy(hash_data.file_name, slaves[i].prev_file_name);  
						hash_data.files_left = num_files - curr_files_read;

						write_to_shm(shm_data.fd, semaphore_read.addr, &hash_data, curr_files_read);
						curr_files_read++;

						// Write hash to file
						fprintf(output, "\nFile: %s Md5: %s Pid: %d\n", hash_data.file_name, hash_data.hash, hash_data.pid);

						if(curr_files_sent < num_files) {
							send_file(slaves[i].app_to_slave[WRITE], &(files[curr_files_sent]));
							slaves[i].prev_file_name = files[curr_files_sent];
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

			break;
	}

	return 0;
}

void exit_handler(int code, void * arr)
{
	// Destroy shared resources
	unlink_shm(SHM_NAME);
	unlink_semaphore(SEM_READ_NAME);
	unlink_semaphore(SEM_CLOSE_NAME);
}

int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

void send_file(int fd, char ** src)
{
	if(write(fd, src, sizeof(char *)) == -1) {
		perror("Writing to slave");
		exit(ERROR_WRITING_PIPE);
	}
}

void write_to_shm(int fd, sem_t * addr, hash_info * hash_data, int curr_files_shm) 
{
	if(pwrite(fd, hash_data, sizeof(hash_info), curr_files_shm * sizeof(hash_info)) == -1){
		perror("Writing to shm");
		exit(ERROR_WRITING_PIPE);
	}

	sem_post(addr);
}
