#include "md5.h"
#include "defs.h"

/* CONSTANTS */

#define FILES_PER_SLAVE 2

#define READ 0 
#define WRITE 1

#define NORMAL 0
#define BACKUP 1

#define SHARED_MEMORY_NAME "md5_shm"
#define SEMAPHORE_NAME "md5_sem"

/* TYPEDEFS */

// Contains data relating to EACH slave
typedef struct slave_info {
	int app_to_slave[2];
	int slave_to_app[2];
	int pid;
	char * prev_file_name;
}slave_info;

// Contains information of pipes from subslave to slave
typedef struct subslave_info{
	int fd_pipe[2];
	fd_set set[2];
}subslave_info;


/* PROTOTYPE */
int slave(int * app_to_slave, int * slave_to_app);


void create_shared_resources(shared_resource_info * resources){
	
	// Create shared memory 
	ERROR_CHECK_KEEP(shm_open(resources->shared_memory_name, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR), resources->shm_fd, -1, "Creating shared memory", ERROR_CREATING_SHM)
	ERROR_CHECK(ftruncate(resources->shm_fd, SHM_SIZE), -1, "Truncating shared memory", ERROR_TRUNCATE_SHM);
	ERROR_CHECK_KEEP(mmap(NULL, SHM_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, resources->shm_fd, 0), resources->mmap_addr, MAP_FAILED, "Mapping shared memory", ERROR_MAPPING_SHM)

	// Create semaphore for shared memory
	ERROR_CHECK_KEEP(sem_open(resources->semaphore_name,  O_CREAT|O_RDWR, S_IRUSR|S_IWUSR, 0),  resources->sem_smh, SEM_FAILED, "Creating semaphore", ERROR_CREATING_SEM)
}

void create_pipes(slave_info * slaves, fd_set * read_fd, int num_slaves){
	
	FD_ZERO(read_fd);

	// Create pipes
	for(int i=0; i < num_slaves; i++){
		D_ERROR_CHECK(pipe(slaves[i].app_to_slave), pipe(slaves[i].slave_to_app), -1, "Creating initial pipes", ERROR_CREATING_SLAVE_PIPES)
		FD_SET(slaves[i].slave_to_app[READ], read_fd);
	}
}

void write_to_shm(shared_resource_info * resources, int pid, char * ans, char * prev_file_name, int num_files,  int curr_files_shm){
	
	// Creating structure that is to be sent
	hash_info hash;		
	hash.pid = pid;
	strcpy(hash.hash, ans);
	strcpy(hash.file_name, prev_file_name);  
	hash.files_left = num_files - curr_files_shm;

	// Write to shared memory using an offset
	pwrite(resources->shm_fd, &hash, sizeof(hash_info), curr_files_shm * sizeof(hash_info));
	
	// Signal that shared memory has item to be recieved
	sem_post(resources->sem_smh);
}

void free_resources(slave_info * slaves, shared_resource_info * resources, int num_slaves){

	// Closing pipes and killing slaves
	for(int i = 0; i < num_slaves; i++){
		D_ERROR_CHECK(close(slaves[i].app_to_slave[WRITE]),close(slaves[i].slave_to_app[READ]), -1, "Closing pipes in app", ERROR_CLOSING_FINAL_PIPES )       
		kill(slaves[i].pid, SIGKILL);
	}

	// // Unmapping and closing of shared memory
	// ERROR_CHECK( munmap(resources->mmap_addr, 3000), -1, "Unmapping shared memory", ERROR_UNMAPPING_SHM)
	// ERROR_CHECK(close(resources->shm_fd), -1, "Closing shared memory", ERROR_CLOSING_SHM)
	// ERROR_CHECK(shm_unlink(resources->shared_memory_name), -1, "Unlinking shared memory",ERROR_UNLINKING_SHM )


	// // Closing semaphore
	// ERROR_CHECK(sem_close(resources->sem_smh), -1, "Closing semaphore", ERROR_CLOSING_SEM)
	//ERROR_CHECK(sem_unlink(resources->semaphore_name), -1, "Closing semaphore", ERROR_CLOSING_SEM)
}


// Add a single fd to a set of fds
void setup_add_select(fd_set * read, fd_set * backup_read, int fd){
	FD_ZERO(read);
	FD_SET(fd, read);
	*backup_read = *read;
}

void setup_subslave_pipes(subslave_info * subslave){
	// Creating pipe connecting stdout (of subslave) to stdin (of slave)
    ERROR_CHECK(pipe(subslave->fd_pipe), -1, "Creating pipe in slave",ERROR_CREATING_SUBSLAVE_PIPES);

	// Redirect IO to pipe
    D_ERROR_CHECK(dup2(subslave->fd_pipe[READ],0), dup2(subslave->fd_pipe[WRITE], 1), -1, "Rediecting IO pipe to slave", ERROR_SETTING_SUBSLAVE_PIPES)

	// Read end pipe from subslave to slave 
	setup_add_select(&(subslave->set[NORMAL]), &(subslave->set[BACKUP]), 0);		//TODO: remplazar el 0 con un define
}

int main(int argc, char * argv[]){
	
	// No files where give execution is halted
	if(argc <= 1){
		return NO_FILES_FOUND;
	}

	/* --- Creation of local variables and necesary resources --- */

	int num_files = argc - 1;
	int num_slaves = ceil((double) num_files / FILES_PER_SLAVE); // TODO: ARREGLAR ESTO
	
	//	Create shared memory and semaphore
	shared_resource_info resources;
	resources.shared_memory_name = SHARED_MEMORY_NAME;
	resources.semaphore_name = SEMAPHORE_NAME;
	create_shared_resources(&resources);

	// Creating pipes for slaves and adding them to the select set
	slave_info slaves[num_slaves];
	fd_set read_fd, backup_read_fd;

	create_pipes(slaves ,&read_fd, num_slaves);

	backup_read_fd = read_fd;


	/* --- Broadcast for vista process --- */

	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	// Broadcast shared memory address
	printf("%s\n", SHARED_MEMORY_NAME);

	// Broadcast sempaphore
	printf("%s\n", SEMAPHORE_NAME);

	sleep(2);
	

	/* --- Creating slaves --- */

	int curr_slave, curr_id = 1;
	for(curr_slave = 0; curr_slave < num_slaves && curr_id != 0 ; curr_slave++) {
		ERROR_CHECK_KEEP(fork(), curr_id, -1, "Forking slaves", ERROR_CREATING_SLAVE)
		
		slaves[curr_slave].pid = curr_id;
	}


	/* --- SLAVE process is running --- */
	if(curr_id == 0){
        slave(slaves[curr_slave-1].app_to_slave, slaves[curr_slave-1].slave_to_app);
	}

	
    /* --- APP process is running --- */
	else{

		/* --- Creation of local variables --- */
		char ans[MD5_SIZE + 1]= {0};
		int curr_files_sent = 1, curr_files_read = 0;	// Ignore first file bc it is the executable's name
		

		/* --- Close useless pipes --- */
		for(int i = 0; i < num_slaves; i++) {
			D_ERROR_CHECK(close(slaves[i].app_to_slave[READ]), close(slaves[i].slave_to_app[WRITE]), -1, "Closing useless pipes in app", ERROR_CLOSING_APP_PIPES)
        }


		/* --- Initial distribution of files to slaves --- */
		for(int i = 0; curr_files_sent < num_slaves; i++){
			ERROR_CHECK(write(slaves[i].app_to_slave[WRITE], &(argv[curr_files_sent]), sizeof(char *)), -1, "Writing to slave", ERROR_WRITING_PIPE ) ;
			slaves[i].prev_file_name = argv[curr_files_sent];
			curr_files_sent++;
		}

		/* --- File data is recieved from subslave process --- */
		while(curr_files_read < num_files) {
			
			// Wait until at least on pipe is full
			ERROR_CHECK(select(FD_SETSIZE, &read_fd, NULL, NULL, NULL), -1, "Select in app", ERROR_SELECT_APP)

			for(int i = 0; i < num_slaves && curr_files_read < num_files; i++) {

				// Select notified that this pipe has something in it
				if(FD_ISSET(slaves[i].slave_to_app[READ], &read_fd)) {

					// Reading pipe from slave to app
					ERROR_CHECK(read(slaves[i].slave_to_app[READ], ans, MD5_SIZE*sizeof(char)), -1, "Read in app", ERROR_READ_SLAVE_PIPE )
					
					// Write hash to shared memory
					write_to_shm(&resources, slaves[i].pid, ans, slaves[i].prev_file_name, num_files, curr_files_read);
					
					curr_files_read++;

					// Send new files if there are any left
					if(curr_files_sent <= num_files){
						ERROR_CHECK(write(slaves[i].app_to_slave[WRITE], &(argv[curr_files_sent]), sizeof(char *)), -1, "Writing to slave", ERROR_WRITING_PIPE ) ;
						slaves[i].prev_file_name = argv[curr_files_sent];
						curr_files_sent++;
					}
				}
			}

			// Restore fds since select is destructive
			read_fd = backup_read_fd;
		}

		// Finished processing, closing pipes and killing slaves	
		free_resources(slaves, &resources, num_slaves);
	}

    return 0;
}


int slave(int * app_to_slave, int * slave_to_app){

	/* --- Creating local variables --- */

	// Slave to app and subslave to slave
	subslave_info sublsave;
	fd_set app_to_slave_set[2];

	char * args[] = { "md5sum", NULL, NULL };	// 2nd arg will be path received from APP
	int finished = 0;					
	char * file_name;
	char output[MD5_SIZE + 1]= {0};		// calculated hash for file 


	/* --- Closing useless pipes and creating new ones --- */

	// Close useless pipes
	D_ERROR_CHECK(close(app_to_slave[WRITE]), close(slave_to_app[READ]),-1, "Closing useless pipes in slave", ERROR_CLOSING_SLAVE_PIPES)
	
	// TODO: cerrar todos los fd que no usamos

	// Read end of pipe between app and slave
	setup_add_select(&(app_to_slave_set[NORMAL]), &(app_to_slave_set[BACKUP]), app_to_slave[READ]);

	// Route pipes from subslave to slave
	setup_subslave_pipes(&sublsave);


	while(!finished) {

        // Wait until father sends something through pipe
		ERROR_CHECK(select(FD_SETSIZE, &(app_to_slave_set[NORMAL]), NULL, NULL, NULL), -1, "Select in slave for reading pipe from app.",ERROR_SELECT_SLAVE)

		app_to_slave_set[NORMAL] = app_to_slave_set[BACKUP];		// Backup to previous state

		// Read what app sent
		ERROR_CHECK(read(app_to_slave[READ], &file_name, sizeof(char *)), -1,"Reading answer from sublsave.",  ERROR_SELECT_APP_TO_SLAVE)				

		int id;
		ERROR_CHECK_KEEP(fork(),id, -1, "Forking subslave", ERROR_CREATING_SUBSLAVE)

		/* SUBSLAVE: Whose job is to execute md5sum */
		if(id == 0) {
			args[1] = file_name;	
			execvp("md5sum", args);
		}
		
		/* SLAVE */
		else {

			// Wait until subslave sends something through pipe
			ERROR_CHECK(select(FD_SETSIZE, &(sublsave.set[NORMAL]), NULL, NULL, NULL), -1, "Select in slave for reading pipe from app.", ERROR_SELECT_SUBSLAVE)
			sublsave.set[NORMAL] = sublsave.set[BACKUP];		// Backup to previous state

			// Read md5 hash
			ERROR_CHECK(read(0, output, MD5_SIZE * sizeof(char)), -1, "Reading answer from sublsave.", ERROR_READ_SUBSLAVE_PIPE)		

			// Flush stdin
			int dump;
			while ((dump = getchar()) != '\n' && dump != EOF);

			wait(NULL); // Wait for subslave

			// Write md5 hash to pipe that goes to app 
			ERROR_CHECK(write(slave_to_app[WRITE], output, MD5_SIZE*sizeof(char)), -1, "Writing hash to app", ERROR_WRITING_PIPE );
		}
	}		
	return 0;
}