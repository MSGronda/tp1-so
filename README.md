# TP1 - Sistemas Operativos

## Requirements
Programs have to be compiled using the chair´s docker container:

```sh
user@linux:~$ docker pull agodio/itba-so:1.0
user@linux:~$ cd tp1-so
user@linux:~$ docker run -v "${PWD}:/root" --security-opt seccomp:unconfined -ti agodio/itba-so:1.0
user@docker:~$ cd root
```

## Compilation Instructions
To compile the program, inside the docker container run:

```sh
user@docker:~/root$ make all
```

## Execution Instructions
There are 3 possible ways to successfully execute the program.

### Case 1
If you just want to calculate the hash of the files run: `./md5 <files>`.  

### Case 2
If you want to calculate the hash and watch the files as they are processed run: `./md5 <files> | ./vista`.

### Case 3
If you want to do the same as case 2 but in different terminals run:  
**Terminal 1**
```sh
./md5 <files>
```
to which the program will output 3 names, this _have to be used as arguments for vista_ in the same order they appear.

**Terminal 2**
```sh
./vista <arg1> <arg2> <arg3>
```

## Results
In any case, results are stored in `respuesta.txt`.

