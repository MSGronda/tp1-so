#define _POSIX_SOURCE
#define _BSD_SOURCE 
#define _XOPEN_SOURCE 501


#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <semaphore.h>

#define FILES_PER_SLAVE 2
#define MD5_SIZE 32
#define MAX_NAME_LENGTH 100

#define READ 0 
#define WRITE 1

#define SHARED_MEMORY_NAME "md5_shm"
#define SEMAPHORE_NAME "md5_sem"

// Error codes
#define ERROR_CREATING_SLAVE_PIPES 1
#define ERROR_CREATING_SLAVE 2
#define ERROR_CLOSING_SLAVE_PIPES 3
#define ERROR_CREATING_SUBSLAVE_PIPES 4
#define ERROR_SETTING_SUBSLAVE_PIPES 5
#define ERROR_CREATING_SUBSLAVE 6
#define ERROR_SELECT_APP 7
#define ERROR_READ_SLAVE_PIPE 8
#define ERROR_CLOSING_APP_PIPES 9
#define ERROR_SELECT_SLAVE 10
#define ERROR_SELECT_APP_TO_SLAVE 11
#define ERROR_SELECT_SUBSLAVE 12
#define ERROR_READ_SUBSLAVE_PIPE 13
#define ERROR_CLOSING_FINAL_PIPES 14

// Contains data relating to each slave
typedef struct slave_info {
	int app_to_slave[2];
	int slave_to_app[2];
	int pid;
	char * prev_file_name;
}slave_info;


typedef struct hash_info{
	int pid;
	char hash[MD5_SIZE + 1];
	char file_name[MAX_NAME_LENGTH];	
}hash_info;



int main(int argc, char * argv[]){

	// Turning off print buffering
	setvbuf(stdout, NULL, _IONBF, 0);

	int num_files = argc - 1;
	int num_slaves = ceil((double) num_files / FILES_PER_SLAVE); // TODO: ARREGLAR ESTO

	//TODO: handle que no me pasen ningun file

	// Create shared memory 
	int shm_fd = shm_open(SHARED_MEMORY_NAME, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
	if(shm_fd == -1){
		perror("Creating shared memory");
		exit(1912931);
	}

	if(ftruncate(shm_fd, 3000) == -1){
		perror("Truncating shared memory");
		exit(1912931);
	}
	if( mmap(NULL, 3000, PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0) == MAP_FAILED ){
		perror("Mapping shared memory");
		exit(1912931);
	}

	// Create semaphore for shared memory
	sem_t * sem_smh = sem_open(SEMAPHORE_NAME,  O_CREAT|O_RDWR, S_IRUSR|S_IWUSR, 0);
	if( sem_smh == SEM_FAILED){
		perror("Creating semaphore");
		exit(34554);
	}

	// Broadcast shared memory address
	printf(SHARED_MEMORY_NAME);
	
	// Broadcast sempaphore
	printf(SEMAPHORE_NAME);

	sleep(2);

	slave_info slaves[num_slaves];
	
	// Grouping all slave to app read fds
	fd_set read_fd, backup_read_fd;
	FD_ZERO(&read_fd);

	// Create pipes
	for(int i=0; i < num_slaves; i++){
		if(pipe(slaves[i].app_to_slave) == -1 || pipe(slaves[i].slave_to_app) == -1) {
			perror("Creating initial pipes");
			exit(ERROR_CREATING_SLAVE_PIPES);
		}

		FD_SET(slaves[i].slave_to_app[READ], &read_fd);	
	}
	backup_read_fd = read_fd;

	// Create slaves
	int curr_slave, curr_id = 1;
	for(curr_slave = 0; curr_slave < num_slaves && curr_id != 0 ; curr_slave++) {
		if((curr_id = fork()) == -1){
			perror("Forking slaves");
			exit(ERROR_CREATING_SLAVE);
		}

		slaves[curr_slave].pid = curr_id;
	}
	// #### TODO: hacerlo mas lindo #### 
	curr_slave--;


	/* SLAVE */
	if(curr_id == 0)
        {
		// Close useless pipes
		if(close(slaves[curr_slave].app_to_slave[WRITE]) == -1 || close(slaves[curr_slave].slave_to_app[READ]) == -1) {
            perror("Closing useless pipes in slave");
            exit(ERROR_CLOSING_SLAVE_PIPES);
        }

		int fd[2];
		char * args[] = { "md5sum", NULL, NULL };	// 2nd arg will be path received from APP

        // Pipe connecting stdout (of subslave) to stdin (of slave)
		if(pipe(fd) == -1) {
			perror("Creating pipe in slave");
			exit(ERROR_CREATING_SUBSLAVE_PIPES);
		}

		int original = dup(1);		// #### REMOVE ####
		// Redirect IO to pipe
        if(dup2(fd[READ],0) == -1 || dup2(fd[WRITE], 1) == -1) {
            perror("Rediecting IO pipe to slave");
            exit(ERROR_SETTING_SUBSLAVE_PIPES);
        }
		
		// TODO: cerrar todos los fd que no usamos
                
		
		int finished = 0;					
		char * file;						// file name 
		char output[MD5_SIZE + 1]= {0};		// calculated hash for file 


		// Read end of pipe between app and slave
		fd_set slave_read_fd, backup_slave_read_fd;
		FD_ZERO(&slave_read_fd);
		FD_SET(slaves[curr_slave].app_to_slave[READ], &slave_read_fd);
		backup_slave_read_fd = slave_read_fd;


		// Read end pipe from subslave to slave 
		fd_set subslave_read_fd, backup_subslave_read_fd;
		FD_ZERO(&subslave_read_fd);
		FD_SET(0, &subslave_read_fd);					//TODO: remplazar el 0 con un define
		backup_subslave_read_fd = subslave_read_fd;

		while(!finished) {

            // Wait until father sends something through pipe
			if(select(FD_SETSIZE, &slave_read_fd, NULL, NULL, NULL) < 0) {
				perror("Select in slave for reading pipe from app.");
				exit(ERROR_SELECT_SLAVE);
			}

			slave_read_fd = backup_slave_read_fd;		// Backup to previous state

			// Read what app sent
			if(read(slaves[curr_slave].app_to_slave[READ], &file, sizeof(char *)) == -1){
				perror("Reading answer from sublsave.");
				exit(ERROR_SELECT_APP_TO_SLAVE);
			}				

			int id;
			if((id = fork()) == -1){
				perror("Forking subslave");
				exit(ERROR_CREATING_SUBSLAVE);
			}

			/* SUBSLAVE: Whose job is to execute md5sum */
			if(id == 0) {
				args[1] = file;	// arg will be filename		
				execvp("md5sum", args);
			}
			

			/* SLAVE */
			else {

				// Wait until subslave sends something through pipe
				if(select(FD_SETSIZE, &subslave_read_fd, NULL, NULL, NULL) < 0) {
					perror("Select in slave for reading pipe from app.");
					exit(ERROR_SELECT_SUBSLAVE);
				}
				subslave_read_fd = backup_subslave_read_fd;		// Backup to previous state

				// Read md5 hash
				if(read(0, output, MD5_SIZE * sizeof(char)) == -1){
					perror("Reading answer from sublsave.");
					exit(ERROR_READ_SUBSLAVE_PIPE);
				}		

				// Flush stdin
				int dump;
				while ((dump = getchar()) != '\n' && dump != EOF);

				wait(NULL); // Wait for subslave

				// #### REMOVE ####
 				dup2(original, 1);
 				printf("\tHijo i=%d manda: %s -> %s\n\n",curr_slave, file,output);
 				dup2(fd[1],1);

 				// Write md5 hash to pipe that goes to app 
 				write(slaves[curr_slave].slave_to_app[WRITE], output, MD5_SIZE*sizeof(char));
  			}		
		}
	}
        /* APP */
	else{
		char ans[MD5_SIZE + 1]= {0};
		int curr_files_sent = 1, curr_files_read = 0, curr_files_shm = 0;	// Ignore first file bc it is the executable's name

		// Close useless pipes
		for(int i = 0; i < num_slaves; i++) {
            if(close(slaves[i].app_to_slave[READ]) == -1 || close(slaves[i].slave_to_app[WRITE]) == -1) {
                perror("Closing useless pipes in app");
                exit(ERROR_CLOSING_APP_PIPES);
            }
		}

		// Initial distribution of files to slaves
		for(int i = 0; curr_files_sent < num_slaves; i++, curr_files_sent++){
			write(slaves[i].app_to_slave[WRITE], &(argv[curr_files_sent]), sizeof(char *));
			slaves[i].prev_file_name = argv[curr_files_sent];
		}

		// Send files until there are no more to process
		while(curr_files_read < num_files) {
			
			// Checking if any of the slaves have a hash
			if(select(FD_SETSIZE, &read_fd, NULL, NULL, NULL) < 0) {
				perror("Select in app");
				exit(ERROR_SELECT_APP);
			}

			for(int i = 0; i < num_slaves && curr_files_read < num_files; i++) {

				// Select notified that this pipe has something in it
				if(FD_ISSET(slaves[i].slave_to_app[READ], &read_fd)) {

					// Reading pipe from slave to app
					if(read(slaves[i].slave_to_app[READ], ans, MD5_SIZE*sizeof(char)) == -1){
						perror("Read in app");
						exit(ERROR_READ_SLAVE_PIPE);
					}
					curr_files_read++;

					printf("Recibi de i=%d : %s\n\n", i ,ans);

					// Create structure to write to shared memory
					hash_info hash;
					hash.pid = slaves[i].pid;
					strcpy(hash.hash, ans);
					strcpy(hash.file_name, slaves[i].prev_file_name);  

					// Write to shared memory using an offset
					pwrite(shm_fd, &hash, sizeof(hash_info), curr_files_shm * sizeof(hash_info));
					curr_files_shm++;
					sem_post(sem_smh);


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

		printf("Cerrado el ciclo original\n");
		
		// Finished processing, closing pipes and killing slaves
		for(int i = 0; i < num_slaves; i++){
            if( close(slaves[i].app_to_slave[WRITE]) == -1 || close(slaves[i].slave_to_app[READ]) == -1) {
                perror("Closing pipes in app");
                exit(ERROR_CLOSING_FINAL_PIPES);
            }

            // TODO: matar al hijo pero de manera linda
            kill(slaves[i].pid, SIGKILL);
		}
		

		printf("Matamos todos los hijos?\n");
	}

        return 0;
}
