#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <math.h>

#define FILES_PER_SLAVE 2
#define READ 0 
#define WRITE 1

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

	fd_set read_fd;
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

		// redireccionamos la entrada y salida estandard al pipe
		dup2(fd[1], 1);	// TODO: chequear error
		dup2(fd[0],0);
		
		// TODO: cerrar todos los fd que no usamos

		int finished=0;
		char salida[33];
		salida[32] = 0;						// null termination

		while(finished==0){
			

			// RECIBIR EL FILE PATH
			while(read(slave_info[i].parent_to_child_read, &file, sizeof(char *)) < 1);
			// TODO: esto es busy waiting? Ver como suspender ejecucion hasta que padre me mande algo

			arg[1] = file;

			// #### creamos sub esclavo que va a correr el md5sum ####
			int id = fork();
			if(id==-1){
				perror("ERROR! Pipes en esclavo!\n");
				exit(4);
			}

			// #### proceso subesclavo #### 
			if(id==0){
				execvp("md5sum", arg);
			}


			// #### proceso esclavo #### 
			else{				

				while(read(0, salida, 32 * sizeof(char)) < 1);		// TODO: esto se puede solucionar con semaforos o estamos forzados a hacerlo asi (en este caso)???

				char caracter;
				while(read(0,&caracter, 1) >1); 	//flushear la entrada TODO: ver hacerlo mas elegante
 				wait(NULL);

			
 				write(slave_info[i].child_to_parent_write, &salida, sizeof(char *));

  			}		
		}
	}


	//  #### proceso padre #### 
	else{
		// cerramos los pipes que no vamos a usar
		for(int j=0; j<num_slaves; j++){
			close(slave_info[i].parent_to_child_read);		// no quiere leer 
			close(slave_info[i].child_to_parent_write);		// no quiere escribir
		}

		while(number_of_files>0){
			//TODO: un ciclo andonde mando nombre de files a esclavos
			for(int i=0;i<num_slaves; i++){
				write(slave_info[i].parent_to_child_write, &(argv[1]), sizeof(char *)); //matado
			}


			//TODO: recibo la respues de ellos usando select y un ciclo de read 
			int select_value = select(num_slaves, &read_fd, NULL, NULL, NULL);
			if(select_value==-1){
				perror("Select");
				exit(5);
			}
			else if(select_value){
				printf("dentro del select\n");
				for(int i=0; i<num_slaves; i++){
					char * respuesta;
					int read_val = read(slave_info[i].child_to_parent_read, &respuesta, sizeof(char *));
					if(read_val == -1){
						perror("Read parent");
						exit(6);
					}
					else if(read_val > 0){
						//TODO: pasar a un buffer
						printf("Recibi del esclavo: %s\n", respuesta);

						number_of_files--;
					}
				}
			}
		}
		
		

		// nos quedamos sin files y ya recibimos todas las
		// esperamos a todos los esclavos 
		for(int j=0; j<num_slaves; ){
			if(wait(NULL)>0)
				j++;
		}
	}
}
