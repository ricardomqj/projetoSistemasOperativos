#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_SIZE 300

typedef struct package {
    char command[MAX_SIZE + 1];
	pid_t pid;
} Package;

void exec_command(int N, char* command){
	printf("entrei em exec_command com argumento command igual a -> %s\n", command);
	pid_t pid;
	int pids[N];
	int i, status, res, j = 0;
	char *cmd[N];

	char *token = strtok(command, " ");
	
	while(token != NULL) {
		cmd[j] = token;
		printf("passei em cdm[%d] o token com valor -> %s\n", j, token);
		token = strtok(NULL, " ");
		j++;
	}
	cmd[j] = NULL;
	execvp(cmd[0], cmd);
	perror("execvp");
}
/*
void add_client(pid_t pid, char *command, int client_id, Client *clients) {
	Client *new_client = (Client*)malloc(sizeof(Client));
	new_client->id = client_id;
	new_client->pid = pid;
	new_client->packages = (Package*)malloc(sizeof(Package));
	strcpy(new_client->packages->command, command);
	new_client->packages->next = NULL;
	new_client->next = NULL;
	clients = new_client;
} */

int main(int argc, char *argv[])
{
	int N = 0;
	int id = 0; // variável usada para associar um id aos clientes
	pid_t pid;
	
	if(mkfifo("fifoOrchCli", 0666) == -1 || mkfifo("fifoCliOrch", 0666) == -1) {
		perror("mkfifo");
		_exit(1);
	}

	int fdFifoOrchCli = open("fifoOrchCli", O_WRONLY);
	int fdFifoCliOrch = open("fifoCliOrch", O_RDONLY);

	while(1) {
		//char buffer[MAX_SIZE + 1];
		//read(fdFifoCliOrch, buffer, MAX_SIZE);
		ssize_t bytes_read;
		pid_t client_pid;
		Package pack;
		bytes_read = read(fdFifoCliOrch, &pack, sizeof(Package));
		if(bytes_read > 0) {
			printf("Estrutura do pack recebido: \n");
			printf("pack.command -> %s\n", pack.command);
			printf("pack.pid -> %d\n", pack.pid);
			id++;
			/*
			char *token = strtok(buffer, " ");
			int i = 0;
			while(token != NULL) {
				token = strtok(NULL, " ");
				i++;
			} */
			pid_t pid = fork();
			if(pid == -1) {
				perror("fork");
				_exit(1);
			} else if(pid == 0) {
				// processo filho
				exec_command(N, pack.command);
				return EXIT_SUCCESS;
			} else {
				// processo pai
				int status;
				wait(&status);
			}
		}
	}

	return 0;
}