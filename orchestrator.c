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

	/*
	for(i = 0; i < N; i++){
		if((pid = fork()) == 0){
			execvp(commands[i], &commands[i]);
			_exit(i);
		} 
	} */
	
	/*
	for(int i = 0; i < N; i++){
		waitpid(pids[i], &status, 0);
	}*/

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
	int i, N = 0;
	int id = 0; // variável usada para associar um id aos clientes
	//char *commands[2];
	pid_t pid;
	
	if(mkfifo("fifoOrchCli", 0666) == -1 || mkfifo("fifoCliOrch", 0666) == -1) {
		perror("mkfifo");
		_exit(1);
	}

	int fdFifoOrchCli = open("fifoOrchCli", O_WRONLY);
	int fdFifoCliOrch = open("fifoCliOrch", O_RDONLY);

	/*
	for(i = 1; i < argc; i++){
		printf("Dentro do ciclo for -> argv[%d] = %s\n", i, argv[i]);
		commands[N] = strdup(argv[i]);
		N++;
	} */

	while(1) {
		char buffer[MAX_SIZE + 1];
		read(fdFifoCliOrch, buffer, MAX_SIZE);
		pid_t client_pid;
		char buffer_copy[MAX_SIZE + 1];
		Package pack;
		read(fdFifoCliOrch, &pack, sizeof(Package));
		printf("Estrutura do pack recebido: \n");
		printf("pack.command -> %s\n", pack.command);
		printf("pack.pid -> %d\n", pack.pid);
		strcpy(buffer_copy, buffer);
		id++;
		char *token = strtok(buffer, " ");
		int i = 0;
		while(token != NULL) {
			token = strtok(NULL, " ");
			i++;
		}
		exec_command(i, pack.command);
	}
	/*
	Package pack = malloc(sizeof(struct package));
	printf("vou ler do fifo\n");
	pid = fork();
	if(pid == -1) {
		perror("fork");
		_exit(1);
	} else if (pid == 0) {
		// código do processo filho
		char buffer[MAX_SIZE + 1];
		char buffer_copy[MAX_SIZE + 1];
		read(fdFifoCliOrch, &buffer, sizeof(pack->command));
		printf("Recebi do client o comando: %s\n", buffer);
		strcpy(buffer_copy, buffer);
		char *token = strtok(buffer, " ");
		int i = 0;
		while(token != NULL) {
			token = strtok(NULL, " ");
			i++;
		}
		printf("antes de entrar em exec_command com '%s'\n", buffer_copy);
		exec_command(i, buffer_copy);

	} else {
		// código do processo pai
		printf("parei de ler do fifo\n");
		printf("Comando recebido do cliente -> %s\n", pack->command);
		free(pack);
	}

	//orchestrator(N, commands[N]);*/

	return 0;
}