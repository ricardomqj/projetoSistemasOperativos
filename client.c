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
#define NOT_EXECUTED 11
#define EXECUTING 12
#define EXECUTED 13
#define EXECUTED_ERROR 14
#define EXECUTE_COMMAND 21
#define EXECUTE_MULT_COMMANDS 22
#define EXECUTE_STATUS 23

typedef struct package {
    int id;
    char command[MAX_SIZE + 1];
	pid_t pid;
    int status;
    int command_type;
    struct package *next;
} Package;

int main(int argc, char *argv[]) {
    int fdFifoOrchCli = open("fifoOrchCli", O_RDONLY);
	int fdFifoCliOrch = open("fifoCliOrch", O_WRONLY);
    pid_t pid;

	if(fdFifoCliOrch == -1 || fdFifoOrchCli == -1) {
		perror("open");
		_exit(1);
	}

    if(strcmp(argv[1], "execute") == 0) {
        if(strcmp(argv[3], "-u") == 0) {
            pid = fork();
            if(pid == -1) {
                perror("fork");
                _exit(1);
            }

            if(pid == 0) {
                // código do processo filho 
                pid_t pid_filho = getpid();
                Package pack;
                strcpy(pack.command, argv[4]);
                pack.pid = pid_filho;
                pack.id = -1;
                pack.command_type = EXECUTE_COMMAND;
                pack.next = NULL;
                printf("Conteudo do package a ser enviado:\n");
                printf("pack.comand -> %s\n", pack.command);
                printf("pack.pid -> %d\n", pack.pid);
                printf("pack.id -> %d\n", pack.id);
                if(write(fdFifoCliOrch, &pack, sizeof(Package)) < 0) {
                    perror("write");
                    _exit(1);
                }
            } 
        } else if(strcmp(argv[3], "-p") == 0) {
            pid = fork();
            if(pid == -1) {
                perror("fork");
                _exit(1);
            } else if(pid == 0) {
                printf("Pipelining not implemented yet...\n");
                _exit(1);
            }
        } else {
            printf("Invalid option. Use '-u' or '-p'...\n");
            return 0;
        }
    } else if(strcmp(argv[1], "status") == 0) {
        pid = fork();
        printf("Entrei no strcmp do STATUS!\n");
        if(pid == -1) {
            perror("fork");
            _exit(1);
        } else if (pid == 0) {
            // código do processo filho
            pid_t pid_filho = getpid();
            Package pack;
            pack.id = -1;
            pack.command_type = EXECUTE_STATUS;
            pack.pid = pid_filho;
            strcpy(pack.command, "STATUS_COMMAND");
            if(write(fdFifoCliOrch, &pack, sizeof(Package)) < 0) {
                perror("write");
                _exit(1);
            }
            Package received_packages[BUFSIZ];
            int num_packages = 0;
            ssize_t bytes_read;
            printf("[STATUS]antes de iniciar leitura no fifo!\n");
            while((bytes_read = read(fdFifoOrchCli, &received_packages[num_packages], sizeof(Package))) > 0) {
                num_packages++;
            }
            printf("[STATUS] leitura do fifo feita!\n");
            // Imprimir os estados dps pacotes recebidos
            printf("Estado atual dos pacotes:\n");
            for(int i = 0; i < num_packages; i++) {
                printf("Package ID: %d\n", received_packages[i].id);
                printf("Command: %d\n", received_packages[i].status); 
                printf("Status: ");
                switch(received_packages[i].status) {
                    case NOT_EXECUTED:
                        printf("NOT_EXECUTED\n");
                        break;
                    case EXECUTING:
                        printf("EXECUTING\n");
                        break;
                    case EXECUTED:
                        printf("EXECUTED\n");
                        break;
                    case EXECUTED_ERROR:
                        printf("EXECUTED_ERROR\n");
                        break;
                    default:
                        printf("Unknown\n");
                        break;
                }
                printf("____________________\n");
            }
        } 
    } else {
        printf("Comando inserido é inválido!\n");
        return 0;
    }

    return 0;
}