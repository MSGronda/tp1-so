#define _POSIX_SOURCE

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define FILES_PER_SLAVE 2
#define MD5_SIZE 32

#define READ 0 
#define WRITE 1

struct slave_info {
	int app_to_slave[2];
	int slave_to_app[2];

	int pid;
};


int 
main(int argc, char * argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	
	int num_files = argc - 1;
	int num_slaves = ceil((double) num_files / FILES_PER_SLAVE); // TODO: ARREGLAR ESTO

        struct slave_info slaves[num_slaves];
	
	fd_set read_fd, backup_read_fd;
	FD_ZERO(&read_fd);

	// Create pipes
	for(int i=0; i < num_slaves; i++){
		if(pipe(slaves[i].app_to_slave) == -1 || pipe(slaves[i].slave_to_app) == -1) {
			perror("Creating initial pipes");
			exit(1);
		}

		FD_SET(slaves[i].slave_to_app[READ], &read_fd);	
	}
	backup_read_fd = read_fd;

	// Create slaves
	int curr_slave, curr_id = 1;
	for(curr_slave = 0; curr_slave < num_slaves && curr_id != 0 ; curr_slave++) {
		if((curr_id = fork()) == -1){
			perror("Forking slaves");
			exit(2);
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
                    exit(3);
                }

		int fd[2];
		char * args[] = { "md5sum", NULL, NULL };	// 2nd arg will be path received from APP

                // Create pipe
		if(pipe(fd) == -1) {
			perror("Creating pipe in slave");
			exit(4);
		}

		int original = dup(1);
		// Redirect IO to pipe
                if(dup2(fd[READ],0) == -1 || dup2(fd[WRITE], 1) == -1) {
                    perror("Rediecting IO pipe to slave");
                    exit(5);
                }
		
		// TODO: cerrar todos los fd que no usamos
                
		int finished = 0;
		char * file;
		char output[MD5_SIZE + 1]= {0};

		while(finished==0) {
                        // Read what app sent
			while(read(slaves[curr_slave].app_to_slave[READ], &file, sizeof(char *)) < 1);			
			//TODO: encontrar forma de hacer esto correctamente!!!!!!!!

			int id;
			if((id = fork()) == -1){
				perror("Forking subslave");
				exit(6);
			}

                        /* SUBSLAVE: Only job is to execute md5sum */
			if(id == 0) {
				args[1] = file;	// arg will be filename		
				execvp("md5sum", args);
			}
                        /* SLAVE */
			else {				
				while(read(0, output, MD5_SIZE * sizeof(char)) < 1);		// TODO: esto se puede solucionar con semaforos o estamos forzados a hacerlo asi (en este caso)???

				// Flush stdin
				int dump;
				while ((dump = getchar()) != '\n' && dump != EOF);

				wait(NULL); // Wait for subslave

 				dup2(original, 1);
 				printf("\tHijo i=%d manda: %s -> %s\n\n",curr_slave, file,output);
 				dup2(fd[1],1);

 				write(slaves[curr_slave].slave_to_app[WRITE], output, MD5_SIZE*sizeof(char));
  			}		
		}
	}
        /* APP */
	else
        {
		char ans[MD5_SIZE + 1]= {0};
		int curr_files_sent = 0, curr_files_read = 0;	// Ignore first file bc it is the executable's name


		// Close useless pipes
		for(int i = 0; i < num_slaves; i++) {
                    if(close(slaves[i].app_to_slave[READ]) == -1 || close(slaves[i].slave_to_app[WRITE]) == -1) {
                        perror("Closing useless pipes in app");
                        exit(3);
                    }
		}

		// Initial distribution of files to slaves
		for(int i = 0; curr_files_sent < num_slaves; i++, curr_files_sent++)
			write(slaves[i].app_to_slave[WRITE], &(argv[curr_files_sent]), sizeof(char *));

		// Send files until there are no more to process
		while(curr_files_read < num_files) {
			if(select(FD_SETSIZE, &read_fd, NULL, NULL, NULL) < 0) {
				perror("Select in app");
				exit(7);
			}

			for(int i = 0; i < num_slaves && curr_files_read < num_files; i++) {
				if(FD_ISSET(slaves[i].slave_to_app[READ], &read_fd)) {
					int read_val = read(slaves[i].slave_to_app[READ], ans, MD5_SIZE*sizeof(char));
					curr_files_read++;

					if(read_val == -1){
						perror("Read in app");
						exit(8);
					}

					printf("Recibi de i=%d, n=%d : %s\n\n", i, read_val,ans);

					// TODO: escribir al buffer de anss

                                        // Send new files if there are any left
					if(curr_files_sent < num_files)
						write(slaves[i].app_to_slave[WRITE], &(argv[curr_files_sent++]), sizeof(char *));
				}
			}

                        // Restore fds because select is destructive
			read_fd = backup_read_fd;
		}

		printf("Cerrado el ciclo original\n");
		
		// Finished processing so we close pipes
		for(int i = 0; i < num_slaves; i++){
                    if( close(slaves[i].app_to_slave[WRITE]) == -1 || close(slaves[i].slave_to_app[READ]) == -1) {
                        perror("Closing pipes in app");
                        exit(9);
                    }

                    // TODO: matar al hijo pero de manera linda
                    kill(slaves[i].pid, SIGKILL);
		}
		

		printf("Matamos todos los hijos?\n");
	}

        return 0;
}
