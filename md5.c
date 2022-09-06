#define _POSIX_SOURCE

#include <math.h>
#include <stdio.h>
#include <signal.h>
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

	// #### creamos los pipes #### 
	for(int i=0; i < num_slaves; i++){
		if(pipe(slaves[i].app_to_slave) == -1 || pipe(slaves[i].slave_to_app) == -1) {
			perror("ERROR! Pipes!\n");
			exit(2);
		}

		FD_SET(slaves[i].slave_to_app[READ], &read_fd);	
	}
	backup_read_fd = read_fd;

	// #### creamos los esclavos #### 
	int curr_slave, currentId = 1;
	for(curr_slave = 0; curr_slave < num_slaves && currentId != 0 ; curr_slave++) {
		// cheque de error
		if((currentId = fork()) == -1){
			perror("ERROR! Fork!\n");
			exit(1);
		}

		slaves[curr_slave].pid = currentId;
	}

	// #### TODO: hacerlo mas lindo #### 
	curr_slave--;


	//  #### proceso esclavo #### 
	if(currentId == 0){
		// cerramos los pipes que no vamos a usar
		close(slaves[curr_slave].app_to_slave[WRITE]);
		close(slaves[curr_slave].slave_to_app[READ]);

		// variables
		char * file;
		int fd[2];
		char * arg[] = {"md5sum", NULL, NULL};	// el segundo elemento va a ser el path que recibimos del padre

		// creamos los pipes para comunicar subesclavo con esclavo
		if(pipe(fd)==-1){
			perror("ERROR! Pipe de slave!\n");
			exit(3);
		}

		int original = dup(1);
		// redireccionamos la entrada y salida estandard al pipe
		dup2(fd[1], 1);	// TODO: chequear error
		dup2(fd[0],0);
		
		// TODO: cerrar todos los fd que no usamos

		int finished=0;
		char salida[MD5_SIZE + 1]= {0};

		while(finished==0){
			
			// leemos lo que le mando
			while(read(slaves[curr_slave].app_to_slave[READ], &file, sizeof(char *)) < 1);			
			//TODO: encontrar forma de hacer esto correctamente!!!!!!!!

			// #### creamos sub esclavo que va a correr el md5sum ####
			int id = fork();
			if(id==-1){
				perror("ERROR! Pipes en esclavo!\n");
				exit(4);
			}

			// #### proceso subesclavo #### 
			if(id==0){
				arg[1] = file;			//le pasamos como argumento el nombre del file
				execvp("md5sum", arg);
			}


			// #### proceso esclavo #### 
			else{				

				while(read(0, salida, MD5_SIZE * sizeof(char)) < 1);		// TODO: esto se puede solucionar con semaforos o estamos forzados a hacerlo asi (en este caso)???

				// flusheamos la entrada estandar 
				int c;
				while ((c = getchar()) != '\n' && c != EOF);

				// lo esperamos al subesclavo 
				wait(NULL);

 				dup2(original, 1);
 				printf("\tHijo i=%d manda: %s -> %s\n\n",curr_slave, file,salida);
 				dup2(fd[1],1);

 				write(slaves[curr_slave].slave_to_app[WRITE], salida, MD5_SIZE*sizeof(char));
  			}		
		}
	}


	//  #### proceso padre #### 
	else{
		char respuesta[MD5_SIZE + 1]= {0};
		int current_file_sent = 0, current_file_read = 0;				// ignoramos el primer file que es el nombre del ejecutable


		// cerramos los pipes que no vamos a usar
		for(int j=0; j<num_slaves; j++){
			close(slaves[j].app_to_slave[READ]);		// no quiere leer 
			close(slaves[j].slave_to_app[WRITE]);		// no quiere escribir
		}

		// Le pasamos inicialmente a todos los esclavos
		for(int i=0 ; current_file_sent<num_slaves; i++, current_file_sent++){
			write(slaves[i].app_to_slave[WRITE], &(argv[current_file_sent]), sizeof(char *));
		}

		// recibimos y mandamos files hasta que no queden mas para procesar
		while(current_file_read < num_files){

			if(select(FD_SETSIZE, &read_fd, NULL, NULL, NULL)<0){
				perror("Select");
				exit(5);
			}

			// 
			for(int i=0; i<num_slaves && current_file_read < num_files ; i++){

				if(FD_ISSET(slaves[i].slave_to_app[READ], &read_fd)){

					int read_val = read(slaves[i].slave_to_app[READ], respuesta, MD5_SIZE*sizeof(char));
					current_file_read++;

					if(read_val == -1){
						perror("Read parent");
						exit(6);
					}


					printf("Recibi de i=%d, n=%d : %s\n\n", i, read_val,respuesta);


					// TODO: escribir al buffer de respuestas


					// ya que termino de procesar el archivo, le pasamos uno nuevo
					// pero solo si quedan cosas para mandar
					if(current_file_sent < num_files)
						write(slaves[i].app_to_slave[WRITE], &(argv[current_file_sent]), sizeof(char *));
						current_file_sent++;	
				}
			}

			// restauramos los fds a los originales
			read_fd = backup_read_fd;
		}

		printf("Cerrado el ciclo original\n");
		
		// una vez que procesamos todo, cerramos por completo los pipes
		for(int j=0; j<num_slaves; j++){
			close(slaves[j].app_to_slave[WRITE]);
			close(slaves[j].slave_to_app[READ]);

			// TODO: matar al hijo pero de manera linda
			kill(slaves[j].pid, SIGKILL);
		}
		

		printf("Matamos todos los hijos?\n");
	}
}
