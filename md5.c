#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <math.h>
#include <signal.h>

#define FILES_PER_SLAVE 2
#define READ 0 
#define WRITE 1
#define MD5_SIZE 32

typedef struct Slave_Info{
	int parent_to_child_read;
	int parent_to_child_write;
	int child_to_parent_read;
	int child_to_parent_write;
	int pid;
}Slave_Info;


int main(int argc, char * argv[]){

	setvbuf(stdout, NULL, _IONBF, 0);
	
	int number_of_files = argc-1;
	int num_slaves = ceil((double)number_of_files / FILES_PER_SLAVE); // TODO: ARREGLAR ESTO

	int parent_to_child[num_slaves][2];
	int child_to_parent[num_slaves][2];
	
	// #### creamos los pipes #### 
	for(int i=0; i<num_slaves; i++){
		// fd[0] = read  | fd[1] = write
		if(pipe(parent_to_child[i]) == -1 || pipe(child_to_parent[i]) == -1){
			perror("ERROR! Pipes!\n");
			exit(2);
		}
	}

	fd_set read_fd, backup_read_fd;
	FD_ZERO(&read_fd);

	// le pasamos los datos al struct
	Slave_Info slave_info[num_slaves];
	for(int i=0; i<num_slaves; i++){
		slave_info[i].parent_to_child_read = parent_to_child[i][READ];
		slave_info[i].parent_to_child_write = parent_to_child[i][WRITE];
		slave_info[i].child_to_parent_read = child_to_parent[i][READ];
		slave_info[i].child_to_parent_write = child_to_parent[i][WRITE];
		FD_SET(slave_info[i].child_to_parent_read, &read_fd);	
	}

	backup_read_fd = read_fd;

	// #### creamos los esclavos #### 
	int i, currentId=1;
	for(i=0; i<num_slaves && currentId!=0 ;i++){
		currentId = fork();

		// cheque de error
		if(currentId == -1){
			perror("ERROR! Fork!\n");
			exit(1);
		}

		slave_info[i].pid = currentId;
	}

	// #### TODO: hacerlo mas lindo #### 
	i--;


	//  #### proceso esclavo #### 
	if(currentId == 0){
		// cerramos los pipes que no vamos a usar
		close(slave_info[i].parent_to_child_write);
		close(slave_info[i].child_to_parent_read);

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
			while(read(slave_info[i].parent_to_child_read, &file, sizeof(char *)) < 1);			
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
 				printf("\tHijo i=%d manda: %s -> %s\n\n",i, file,salida);
 				dup2(fd[1],1);

 				write(slave_info[i].child_to_parent_write, salida, MD5_SIZE*sizeof(char));
  			}		
		}
	}


	//  #### proceso padre #### 
	else{
		char respuesta[MD5_SIZE + 1]= {0};
		int current_file_sent = 0, current_file_read = 0;				// ignoramos el primer file que es el nombre del ejecutable


		// cerramos los pipes que no vamos a usar
		for(int j=0; j<num_slaves; j++){
			close(slave_info[j].parent_to_child_read);		// no quiere leer 
			close(slave_info[j].child_to_parent_write);		// no quiere escribir
		}

		// Le pasamos inicialmente a todos los esclavos
		for(int i=0 ; current_file_sent<num_slaves; i++, current_file_sent++){
			write(slave_info[i].parent_to_child_write, &(argv[current_file_sent]), sizeof(char *));
		}

		// recibimos y mandamos files hasta que no queden mas para procesar
		while(current_file_read < number_of_files){

			if(select(FD_SETSIZE, &read_fd, NULL, NULL, NULL)<0){
				perror("Select");
				exit(5);
			}

			// 
			for(int i=0; i<num_slaves && current_file_read < number_of_files ; i++){

				if(FD_ISSET(slave_info[i].child_to_parent_read, &read_fd)){

					int read_val = read(slave_info[i].child_to_parent_read, respuesta, MD5_SIZE*sizeof(char));
					current_file_read++;

					if(read_val == -1){
						perror("Read parent");
						exit(6);
					}


					printf("Recibi de i=%d, n=%d : %s\n\n", i, read_val,respuesta);


					// TODO: escribir al buffer de respuestas


					// ya que termino de procesar el archivo, le pasamos uno nuevo
					// pero solo si quedan cosas para mandar
					if(current_file_sent < number_of_files)
						write(slave_info[i].parent_to_child_write, &(argv[current_file_sent]), sizeof(char *));
						current_file_sent++;	
				}
			}

			// restauramos los fds a los originales
			read_fd = backup_read_fd;
		}

		printf("Cerrado el ciclo original\n");
		
		// una vez que procesamos todo, cerramos por completo los pipes
		for(int j=0; j<num_slaves; j++){
			close(slave_info[j].parent_to_child_write);
			close(slave_info[j].child_to_parent_read);

			// TODO: matar al hijo pero de manera linda
			kill(slave_info[j].pid, SIGKILL);
		}
		

		printf("Matamos todos los hijos?\n");
	}
}
