#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAX_SIZE 300

typedef struct package {
    char command[MAX_SIZE + 1];
	pid_t pid;
} Package;

int main(int argc, char *argv[]) {
    
    int fdFifoOrchCli = open("fifoOrchCli", O_RDONLY);
	int fdFifoCliOrch = open("fifoCliOrch", O_WRONLY);
    pid_t pid;

    if (argc < 5) { // Verifique se há argumentos suficientes
        perror("argc");
        _exit(1);
    }

	if(fdFifoCliOrch == -1 || fdFifoOrchCli == -1) {
		perror("open");
		_exit(1);
	}

    if(strcmp(argv[1], "execute") == 0) {
        if(strcmp(argv[3], "-u") == 0) {
            //Package pack = malloc(sizeof(struct package));
            //strcpy(pack->command, argv[4]);
            //printf("guardei em pack->command : [%s]\n", pack->command);
            pid = fork();
            if(pid == -1) {
                perror("fork");
                _exit(1);
            }

            if(pid == 0) {
                // código do processo filho 
                pid_t pid_filho = getpid();
                if(write(fdFifoCliOrch, &pid_filho, sizeof(pid_t)) < 0) {
                    perror("write");
                    _exit(1);
                }
                Package pack;
                strcpy(pack.command, argv[4]);
                pack.pid = pid_filho;
                printf("Conteudo do package a ser enviado:\n");
                printf("pack.comand -> %s\n", pack.command);
                printf("pack.pid -> %d\n", pack.pid);
                if(write(fdFifoCliOrch, &pack, sizeof(Package)) < 0) {
                    perror("write");
                    _exit(1);
                }
                
                
                //printf("vou envar para o orchestrator o command -> %s\n", pack->command);
                //printf("sizeof(pack->command) = %d\n", (int)sizeof(pack->command));
                /*if(write(fdFifoCliOrch, pack->command, sizeof(pack->command)) < 0) {
                    perror("write");
                    _exit(1);
                } */
            } 
        }
    }

}