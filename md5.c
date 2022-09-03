#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_SLAVES 5


int main(int argc, char * argv[]){
	
	int num_slaves = NUM_SLAVES;
	
	int parent_to_child[num_slaves][2];
	int child_to_parent[num_slaves][2];
	int pids[num_slaves]; 

	// #### creamos los pipes #### 
	for(int i=0; i<num_slaves; i++){
		// fd[0] = read  | fd[1] = write
		if(pipe(parent_to_child[i]) == -1 || pipe(child_to_parent[i]) == -1){
			perror("ERROR! Pipes!\n");
			exit(2);
		}
	}

	// #### creamos los esclavos #### 
	int i, currentId=1;
	for(i=0; i<num_slaves && currentId!=0 ;i++){
		currentId = fork();

		// CHEQUEO DE ERROR
		if(currentId == -1){
			perror("ERROR! Fork!\n");
			exit(1);
		}

		pids[i] = currentId;
	}

	// #### TODO: hacerlo mas lindo #### 
	i--;


	//  #### proceso esclavo #### 
	if(currentId == 0){
		// cerramos los pipes que no vamos a usar
		close(parent_to_child[i][1]);		// no quiere escribir
		close(child_to_parent[i][0]);		// no quiere leer

		char * file;

		int fd[2];
		char * arg[] = {"md5sum", NULL, NULL};
		if(pipe(fd)==-1){
			perror("ERROR! Pipe de slave!\n");
			exit(3);
		}
		dup2(fd[1], 1);	// TODO: chequear error
		dup2(fd[0],0);

		// TODO: cerrar todos los fd que no usamos

		int finished=0;
		while(!finised){
			// RECIBIR EL FILE PATH
			// while(read(parent_to_child[i][0], file, ???) < 1){;
			// TODO: esto es busy waiting? Ver como suspender ejecucion hasta que padre me mande algo

			arg[2] = file;

			int id = fork();

			if(id==-1){
				perror("ERROR! Pipes en esclavo!\n");
				exit(4);
			}

			// proceso esclavo
			if(id==0){
				execvp("md5sum", arg);
			}

			// proceso hijo de esclavo
			else{				
				char salida[33] = {0};
				while(read(0,salida, 32) < 0);

				char caracter;
				while(read(0,&caracter, 1) >1); 	//flushear la entrada TODO: ver hacerlo mas elegante
 				wait(NULL);
			}		
		}
	}


	//  #### proceso padre #### 
	else{
		// cerramos los pipes que no vamos a usar
		for(int j=0; j<num_slaves; j++){
			close(parent_to_child[j][0]);		// no quiere leer 
			close(child_to_parent[j][1]);		// no quiere escribir

		}


		// #### esperamos a todos los hijos #### 
		for(int j=0; j<num_slaves; ){
			if(wait(NULL)>0)
				j++;
		}
	}
}
