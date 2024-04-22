#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>

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

typedef struct client {
	pid_t client_pid;
	Package *packages;
	struct client *next;
} Client;


void update_package_status(Client **head, int status, int id) {
	Client *current = *head;
	int updated = 0;
	while(current != NULL && !updated) {
		Package *currentPackage = current->packages;
		while(currentPackage != NULL && !updated) {
			if(currentPackage->id == id) {
				updated = 1;
				currentPackage->status = status;
			}
		}
	}
}

void add_package(Client **head, Package pack, int currentId) {
	printf("Entrei no add_package!\n");
	Client *current = *head;
	while(current != NULL) {
		if(current->client_pid == pack.pid) {
			Package *new_package = (Package *)malloc(sizeof(Package));
			if(new_package == NULL) {
				perror("malloc");
				_exit(1);
			}
			strcpy(new_package->command, pack.command);
			new_package->id = currentId;
			new_package->pid = pack.pid;
			new_package->status = NOT_EXECUTED;
			new_package->command_type = pack.command_type;
			new_package->next = current->packages;
			current->packages = new_package;
			printf("Added package with the following values:\n");
			printf("new_package->id: %d\n", new_package->id);
			printf("new_package->pid: %d\n", new_package->pid);
			printf("new_package->command: %s\n", new_package->command);
			printf("new_package->command_type: %d\n", new_package->command_type);
			printf("_________________________\n");
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
	new_package->id = currentId;
	new_package->pid = pack.pid;
	new_package->status = NOT_EXECUTED;
	new_package->command_type = pack.command_type;
	new_package->next = NULL;
	new_node->packages = new_package;
	new_node->client_pid = pack.pid;
	new_node->next = *head;
	*head = new_node;
	printf("Added package with the following values:\n");
	printf("new_package->id: %d\n", new_package->id);
	printf("new_package->pid: %d\n", new_package->pid);
	printf("new_package->command: %s\n", new_package->command);
	printf("new_package->command_type: %d\n", new_package->command_type);
	printf("new_node->client_pid: %d\n", new_node->client_pid);
	printf("_________________________\n");
}

// Função para imprimir todos os pacotes na lista de clientes
void print_clients(Client *head) {
    printf("\n_____________________\nLista de comandos recebidos:\n");
	//printf("Entrei no print_client com um head com head->client_pid: %d\n", head->client_pid);
    Client *current = head;
    while (current != NULL) {
        printf("Recebido do cliente com PID %d\n", current->client_pid);
		Package *current_package = current->packages;
		while(current_package != NULL) {
			printf("Client PID: %d\n", current_package->pid);
			printf("Command: %s\n", current_package->command);
			printf("Id: %d\n", current_package->id);
			printf("Status: %d\n", current_package->status);
			printf("Command Type: %d\n", current_package->command_type);
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
			printf("pack.id -> %d\n", pack.id);
			printf("pack.command -> %s\n", pack.command);
			printf("pack.pid -> %d\n", pack.pid);
			printf("pack.command_type -> %d\n", pack.command_type);
			id++;
			//add_package(&client_list, pack);
			//print_clients(client_list);
			pid_t pid = fork();
			if(pid == -1) {
				perror("fork");
				_exit(1);
			} else if(pid == 0) {
				if(pack.command_type == EXECUTE_COMMAND) {
					printf("if command_type == EXECUTE_COMMAND, VOU ENTRAR EM ADD_PACKAGE\n");
					add_package(&client_list, pack, id);
					exec_command(N, pack.command);
					return EXIT_SUCCESS;
				} else if(pack.command_type == EXECUTE_STATUS) {
					// Enviar todos os pacotes de volta ao cliente
					Client *current_client = client_list;
					while(current_client != NULL) {
						Package *current_package = current_client->packages;
						while(current_package != NULL) {
							// Mandar o pacote de volta para o cliente
							if(write(fdFifoOrchCli, current_package, sizeof(Package)) < 0) {
								perror("write");
								_exit(1);
							}
							current_package = current_package->next;
						}
						current_client == current_client->next;
					}
				}
			} else {
				// processo pai
				int status;
				waitpid(pid, &status, 0);
				if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
					printf("vou entrar no update_package_status\n");
					print_clients(client_list);
					printf("depois do print_clients\n");
					update_package_status(&client_list, EXECUTED, id);
					printf("Estado do package alterado para EXECUTED!\n");
					printf("A imprimir os pacotes dos clientes...\n");
					print_clients(client_list);
					printf("depois do print_clients\n");
				} else {
					update_package_status(&client_list, EXECUTED_ERROR, id);
					printf("Estado do package alterado para EXECUTED_ERROR!\n");
					printf("A imprimir os pacotes dos clientes...\n");
					print_clients(client_list);
				}
			}
		}
	}

	return 0;
}