#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define FILES_PER_SLAVE 5
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
	
	int number_of_files = argc-1;
	int num_slaves = number_of_files / FILES_PER_SLAVE; // casteo??

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

	// le pasamos los datos al struct
	Slave_Info slave_info[num_slaves];
	for(int i=0; i<num_slaves; i++){
		slave_info[i].parent_to_child_read = parent_to_child[i][READ];
		slave_info[i].parent_to_child_write = parent_to_child[i][WRITE];
		slave_info[i].child_to_parent_read = child_to_parent[i][READ];
		slave_info[i].child_to_parent_write = child_to_parent[i][WRITE];
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
		while(!finished){
			// RECIBIR EL FILE PATH
			// while(read(parent_to_child[i][0], file, ???) < 1){;
			// TODO: esto es busy waiting? Ver como suspender ejecucion hasta que padre me mande algo
			arg[2] = file;


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
				char salida[33];
				salida[32] = 0;				// null termination
				while(read(0,salida, 32) < 0);		// TODO: esto se puede solucionar con semaforos o estamos forzados a hacerlo asi (en este caso)???

				char caracter;
				while(read(0,&caracter, 1) >1); 	//flushear la entrada TODO: ver hacerlo mas elegante
 				wait(NULL);

 				//TODO: escribirle al viejo la respuesta 
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

			//TODO: recibo la respues de ellos usando select y un ciclo de read 
			//select(child_to_parent)

		}
		//TODO: un ciclo andonde mando nombre de files a esclavos
		

		// nos quedamos sin files y ya recibimos todas las
		// esperamos a todos los esclavos 
		for(int j=0; j<num_slaves; ){
			if(wait(NULL)>0)
				j++;
		}
	}
}
