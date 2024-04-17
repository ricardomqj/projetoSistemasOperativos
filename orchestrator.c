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
	struct package *next;
} Package;

typedef struct client {
	pid_t client_pid;
	Package *packages;
	struct client *next;
} Client;


void add_package(Client **head, Package pack) {
    Client *current = *head;
	while(current != NULL) {
		if(current->client_pid == pack.pid) {
			Package *new_package = (Package *)malloc(sizeof(Package));
			if(new_package == NULL) {
				perror("malloc");
				_exit(1);
			}
			strcpy(new_package->command, pack.command);
			new_package->pid = pack.pid;
			new_package->next = current->packages;
			current->packages = new_package;
			return;
		}
		current = current->next;
	}

	// Caso não encontre um cliente com o PID correspondente, cria um novo cliente
	Client *new_node = (Client *)malloc(sizeof(Client));
	Package *new_package = (Package *)malloc(sizeof(Package));
	if(new_node == NULL || new_package == NULL) {
		perror("malloc");
		_exit(1);
	}
	strcpy(new_package->command, pack.command);
	new_package->pid = pack.pid;
	new_package->next = NULL;
	new_node->packages = new_package;
	new_node->client_pid = pack.pid;
	new_node->next = *head;
	*head = new_node;
}

// Função para imprimir todos os pacotes na lista de clientes
void print_clients(Client *head) {
    printf("\n_____________________\nLista de comandos recebidos:\n");
    Client *current = head;
    while (current != NULL) {
        printf("Recebido do cliente com PID %d\n", current->client_pid);
		Package *current_package = current->packages;
		while(current_package != NULL) {
			printf("Client PID: %d\n", current_package->pid);
			printf("Command: %s\n", current_package->command);
			printf("____________________\n");
			current_package = current_package->next;
		}
        current = current->next;
		printf("_______________________\n");
    }
}

void exec_command(int N, char* command){
	pid_t pid;
	int pids[N];
	int i, status, res, j = 0;
	char *cmd[N];

	char *token = strtok(command, " ");
	
	while(token != NULL) {
		cmd[j] = token;
		token = strtok(NULL, " ");
		j++;
	}
	cmd[j] = NULL;
	execvp(cmd[0], cmd);
	perror("execvp");
	_exit(1);
}

int main(int argc, char *argv[])
{
	int N = 0;
	int id = 0; // variável usada para associar um id aos clientes
	pid_t pid;

	Client *client_list = NULL;

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
			//add_package(&client_list, pack);
			//print_clients(client_list);
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
				waitpid(pid, &status, 0);
				if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
					add_package(&client_list, pack);
					print_clients(client_list);
				}
			}
		}
	}

	return 0;
}