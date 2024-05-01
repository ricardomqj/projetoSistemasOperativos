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
		current = current->next;
	}
}

void add_package(Client **head, Package pack, int currentId) {
	printf("Entrei no add_package!\n");
	Client *current = *head;
	int i = 0;
	while(current != NULL) {
		i++;
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
			return;
		}
		current = current->next;
	}
	if(i == 0) printf("client_list given to add_package function is NULL\n");

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
}

// Função para imprimir todos os pacotes na lista de clientes
void print_clients(Client *head) {
    int i = 0;
	printf("\n_____________________\nLista de comandos recebidos:\n");
    //printf("Entrei na funcao print clients com a head -> %d\n", head->client_pid); // da seg fault
	Client *current = head;
    while (current != NULL) {
        printf("Recebido do cliente com PID %d\n", current->client_pid);

		Package *current_package = current->packages;
		while(current_package != NULL) {
			printf("Package inside the client with pid: %d\n", current->client_pid);
			printf("Client PID: %d\n", current_package->pid);
			printf("Command: %s\n", current_package->command);
			printf("Id: %d\n", current_package->id);
			printf("Status: %d\n", current_package->status);
			printf("Command Type: %d\n", current_package->command_type);
			printf("____________________\n");
			current_package = current_package->next;
			i++;
		}
        current = current->next;
		printf("_______________________\n");
    }
	printf("[print_clients] numero pacotes lidos: %d\n", i);
}

void send_status(Client *client_list, int fdFifoOrchCli) {
	Client *current_client = client_list;
	while(current_client != NULL) {
		printf("[send_status] entrei no while exterior\n");
		Package *current_package = current_client->packages;
		while(current_package != NULL) {
			printf("[send_status] entrei no while interior\n");
			write(fdFifoOrchCli, current_package, sizeof(Package));
			current_package = current_package->next;
		}
		current_client = current_client->next;
	}
	close(fdFifoOrchCli);
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

	if(mkfifo("tmp/fifoCliOrch", 0666) == -1) {
		perror("mkfifo");
		_exit(1);
	}

	int fdFifoCliOrch = open("tmp/fifoCliOrch", O_RDONLY);

	while(1) {
		ssize_t bytes_read;
		pid_t client_pid;
		Package pack;
		bytes_read = read(fdFifoCliOrch, &pack, sizeof(Package));
		if(bytes_read > 0) {
			pid_t pid_client = pack.pid;
			printf("Estrutura do pack recebido: \n");
			printf("pack.id -> %d\n", pack.id);
			printf("pack.command -> %s\n", pack.command);
			printf("pack.pid -> %d\n", pack.pid);
			printf("pack.command_type -> %d\n", pack.command_type);
			id++;
			
			int pipefd[2];
			if(pipe(pipefd) == -1) {
				perror("pipe");
				_exit(1);
			}

			pid_t pid = fork();
			if(pid == -1) {
				perror("fork");
				_exit(1);
			} else if(pid == 0) { // código do processo filho
				if(pack.command_type == EXECUTE_COMMAND) {
					close(pipefd[0]);
					printf("if command_type == EXECUTE_COMMAND, VOU ENTRAR EM ADD_PACKAGE\n");
					//add_package(&client_list, pack, id); // enviar antes ao pai e o pai é que faz o add package
					printf("[processo_filho]: vou escrever no pipe\n");
					write(pipefd[1], &pack, sizeof(Package));
					close(pipefd[1]);
					printf("[proc. filho]Vou dar exec_command\n");
					char clientFifo[32];
					sprintf(clientFifo, "tmp/fifo%d", pid_client);
					int fdFifoOrchCli = open(clientFifo, O_WRONLY);
					if(fdFifoOrchCli == -1) {
						perror("open");
						_exit(1);
					}
					if(write(fdFifoOrchCli, &id, sizeof(int)) < 0) {
						perror("write");
						_exit(1);
					}
					close(fdFifoOrchCli);
					exec_command(N, pack.command);
					return EXIT_SUCCESS;
				} else if(pack.command_type == EXECUTE_STATUS) {
					printf("Recebi um pedido de status do cliente\n");
					char clientFifo[32];
					sprintf(clientFifo, "tmp/fifo%d", pid_client);
					int fdFifoOrchCli = open(clientFifo, O_WRONLY);
					if(fdFifoOrchCli == -1) {
						perror("open");
						_exit(1);
					}
					printf("Vou entrar na função de enviar status para o cliente\n");
					send_status(client_list, fdFifoOrchCli);
				}
			} else { // processo pai
				close(pipefd[1]);
				Package pack_buffer;
				int status;
				printf("[processo-pai] vou ler do pipe\n");
				read(pipefd[0], &pack_buffer, sizeof(Package));
				printf("li do pipe um package com os seguinte dados:\n");
				printf("pack_buffer->command: %s\n", pack_buffer.command);
				printf("pack_buffer->command_type: %d\n", pack_buffer.command_type);
				printf("pack_buffer->id: %d\n", pack_buffer.id);
				printf("pack_buffer->pid: %d\n", pack_buffer.pid);
				printf("pack_buffer->status: %d\n", pack_buffer.status);
				close(pipefd[0]);
				printf("vou adicionar o package ao client_list\n");
				add_package(&client_list, pack_buffer, id);
				printf("A imprimir os pacotes dos clientes...\n");
				print_clients(client_list);
				printf("depois do print_clients\n");
				waitpid(pid, &status, 0);
				if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
					// o client list só não é null para o processo filho
					update_package_status(&client_list, EXECUTED, id);
					printf("Estado do package alterado para EXECUTED!\n");
					print_clients(client_list);
					printf("depois do print_clients após alteração do status do programa\n");
				} else {
					update_package_status(&client_list, EXECUTED_ERROR, id);
					printf("Estado do package alterado para EXECUTED_ERROR!\n");
					printf("A imprimir os pacotes dos clientes...\n");
					print_clients(client_list);
				}
				close(pipefd[0]);
			}
		}
	}

	return 0;
}