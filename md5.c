#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#define NUM_SLAVES 5


int main(int argc, char * argv[]){
	int pids[NUM_SLAVES]; 

	int num_slaves = NUM_SLAVES;
	int parent_to_child[num_slaves][2];
	int child_to_parent[num_slaves][2];

	// creamos los pipes
	for(int i=0; i<num_slaves; i++){
		// fd[0] = read  | fd[1] = write
		if(pipe(parent_to_child[i]) == -1 || pipe(child_to_parent[i]) == -1){
			perror("ERROR! Pipes!\n");
			exit(2);
		}
	}

	// creamos los esclavos
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
	i--;

	// proceso hijo
	if(currentId == 0){
		close(parent_to_child[i][1]);		// no quiere escribir
		close(child_to_parent[i][0]);		// no quiere leer
	}
	// proceso padre
	else{
		// cerramos los pipes que no vamos a usar
		for(int j=0; j<num_slaves; j++){
			close(parent_to_child[j][0]);		// no quiere leer 
			close(child_to_parent[j][1]);		// no quiere escribir
		}
		printf("soy padre!\n");
	}
}
