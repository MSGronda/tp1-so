#include "md5.h"
#include "defs.h"

/* CONSTANTS */

#define FILES_PER_SLAVE 2

#define READ 0 
#define WRITE 1

#define SHARED_MEMORY_NAME "md5_shm"
#define SEMAPHORE_NAME "md5_sem"

/* TYPEDEFS */

// Contains data relating to each slave
typedef struct slave_info {
	int app_to_slave[2];
	int slave_to_app[2];
	int pid;
	char * prev_file_name;
}slave_info;

// Contains data of the shared memory and the semaphore
typedef struct shared_resource_info {
	int shm_fd;
	void * mmap_addr;
	sem_t * sem_smh;
}shared_resource_info;

/* PROTOTYPE */
int slave(int * app_to_slave, int * slave_to_app);


void create_shared_resources(shared_resource_info * resources){
	// Create shared memory 
	ERROR_CHECK_KEEP(shm_open(SHARED_MEMORY_NAME, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR), resources->shm_fd, -1, "Creating shared memory", ERROR_CREATING_SHM)
	ERROR_CHECK(ftruncate(resources->shm_fd, 3000), -1, "Truncating shared memory", ERROR_TRUNCATE_SHM);
	ERROR_CHECK_KEEP(mmap(NULL, 3000, PROT_READ|PROT_WRITE, MAP_SHARED, resources->shm_fd, 0), resources->mmap_addr, MAP_FAILED, "Mapping shared memory", ERROR_MAPPING_SHM)

	// Create semaphore for shared memory
	ERROR_CHECK_KEEP(sem_open(SEMAPHORE_NAME,  O_CREAT|O_RDWR, S_IRUSR|S_IWUSR, 0),  resources->sem_smh, SEM_FAILED, "Creating semaphore", ERROR_CREATING_SEM)
}

void create_pipes(slave_info * slaves, fd_set * read_fd, int num_slaves){
	FD_ZERO(read_fd);

	// Create pipes
	for(int i=0; i < num_slaves; i++){
		D_ERROR_CHECK(pipe(slaves[i].app_to_slave), pipe(slaves[i].slave_to_app), -1, "Creating initial pipes", ERROR_CREATING_SLAVE_PIPES)
		FD_SET(slaves[i].slave_to_app[READ], read_fd);
	}
}

void write_to_shm(shared_resource_info * resources, int pid, char * ans, char * prev_file_name, int num_files,  int * curr_files_shm){
	hash_info hash;		
	hash.pid = pid;
	strcpy(hash.hash, ans);
	strcpy(hash.file_name, prev_file_name);  
	hash.files_left = num_files - (*curr_files_shm);

	// Write to shared memory using an offset
	pwrite(resources->shm_fd, &hash, sizeof(hash_info), (*curr_files_shm) * sizeof(hash_info));
	
	sem_post(resources->sem_smh);
	(*curr_files_shm)++;
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


	/* --- Slave process is running --- */
	if(curr_id == 0){
        slave(slaves[curr_slave-1].app_to_slave, slaves[curr_slave-1].slave_to_app);
	}

	
    /* --- App process is running --- */
	else{

		/* --- Creation of local variables --- */
		char ans[MD5_SIZE + 1]= {0};
		int curr_files_sent = 1, curr_files_read = 0, curr_files_shm = 0;	// Ignore first file bc it is the executable's name
		

		/* --- Close useless pipes --- */
		for(int i = 0; i < num_slaves; i++) {
			D_ERROR_CHECK(close(slaves[i].app_to_slave[READ]), close(slaves[i].slave_to_app[WRITE]), -1, "Closing useless pipes in app", ERROR_CLOSING_APP_PIPES)
        }


		/* --- Initial distribution of files to slaves --- */
		for(int i = 0; curr_files_sent < num_slaves; i++, curr_files_sent++){
			// TODO: chequeo de errores
			write(slaves[i].app_to_slave[WRITE], &(argv[curr_files_sent]), sizeof(char *));
			slaves[i].prev_file_name = argv[curr_files_sent];
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
					curr_files_read++;

					// Write hash to shared memory
					write_to_shm(&resources, slaves[i].pid, ans, slaves[i].prev_file_name, num_files, &curr_files_shm);
					

					// Send new files if there are any left
					if(curr_files_sent <= num_files){
						write(slaves[i].app_to_slave[WRITE], &(argv[curr_files_sent]), sizeof(char *));
						slaves[i].prev_file_name = argv[curr_files_sent];
						curr_files_sent++;
					}
				}
			}

			// Restore fds because select is destructive
			read_fd = backup_read_fd;
		}
		
		// Finished processing, closing pipes and killing slaves
		for(int i = 0; i < num_slaves; i++){
			D_ERROR_CHECK(close(slaves[i].app_to_slave[WRITE]),close(slaves[i].slave_to_app[READ]), -1, "Closing pipes in app", ERROR_CLOSING_FINAL_PIPES )
            
            // TODO: matar al hijo pero de manera linda
            kill(slaves[i].pid, SIGKILL);
		}
		
	}

	// // Unmapping and closing of shared memory
	// ERROR_CHECK( munmap(resources.mmap_addr, 3000), -1, "Unmapping shared memory", ERROR_UNMAPPING_SHM)
	// ERROR_CHECK( shm_unlink(SHARED_MEMORY_NAME), -1, "Unlinking shared memory",ERROR_UNLINKING_SHM )
	// ERROR_CHECK(close(resources.shm_fd), -1, "Closing shared memory", ERROR_CLOSING_SHM)

	// // Closing semaphore
	// ERROR_CHECK(sem_close(resources.sem_smh), -1, "Closing semaphore", ERROR_CLOSING_SEM)

    return 0;
}


int slave(int * app_to_slave, int * slave_to_app){
	// Close useless pipes
	D_ERROR_CHECK(close(app_to_slave[WRITE]), close(slave_to_app[READ]),-1, "Closing useless pipes in slave", ERROR_CLOSING_SLAVE_PIPES)

	int fd[2];
	char * args[] = { "md5sum", NULL, NULL };	// 2nd arg will be path received from APP

    // Pipe connecting stdout (of subslave) to stdin (of slave)
    ERROR_CHECK(pipe(fd), -1, "Creating pipe in slave",ERROR_CREATING_SUBSLAVE_PIPES);
	
	// Redirect IO to pipe
    D_ERROR_CHECK(dup2(fd[READ],0), dup2(fd[WRITE], 1), -1, "Rediecting IO pipe to slave", ERROR_SETTING_SUBSLAVE_PIPES)

	// TODO: cerrar todos los fd que no usamos
	
	int finished = 0;					
	char * file_name;
	char output[MD5_SIZE + 1]= {0};		// calculated hash for file 


	// Read end of pipe between app and slave
	fd_set slave_read_fd, backup_slave_read_fd;
	FD_ZERO(&slave_read_fd);
	FD_SET(app_to_slave[READ], &slave_read_fd);
	backup_slave_read_fd = slave_read_fd;


	// Read end pipe from subslave to slave 
	fd_set subslave_read_fd, backup_subslave_read_fd;
	FD_ZERO(&subslave_read_fd);
	FD_SET(0, &subslave_read_fd);					//TODO: remplazar el 0 con un define
	backup_subslave_read_fd = subslave_read_fd;

	while(!finished) {

        // Wait until father sends something through pipe
		ERROR_CHECK(select(FD_SETSIZE, &slave_read_fd, NULL, NULL, NULL), -1, "Select in slave for reading pipe from app.",ERROR_SELECT_SLAVE)

		slave_read_fd = backup_slave_read_fd;		// Backup to previous state

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
			ERROR_CHECK(select(FD_SETSIZE, &subslave_read_fd, NULL, NULL, NULL), -1, "Select in slave for reading pipe from app.", ERROR_SELECT_SUBSLAVE)
			subslave_read_fd = backup_subslave_read_fd;		// Backup to previous state

			// Read md5 hash
			ERROR_CHECK(read(0, output, MD5_SIZE * sizeof(char)), -1, "Reading answer from sublsave.", ERROR_READ_SUBSLAVE_PIPE)		

			// Flush stdin
			int dump;
			while ((dump = getchar()) != '\n' && dump != EOF);

			wait(NULL); // Wait for subslave

			// Write md5 hash to pipe that goes to app 
			//TODO: chequeo de error
			write(slave_to_app[WRITE], output, MD5_SIZE*sizeof(char));
		}
	}		
	return 0;
}