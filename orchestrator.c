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
#define MAX_ARGS 10
#define MAX_CMDS 5

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

// Funçãp que divide string do comando em comandos separados
int split_cmds(char *input, char *cmds[]) {
	int i = 0;
	char *token = strtok(input, "|");
	while(token != NULL) {
		cmds[i++] = token;
		token = strtok(NULL, "|");
	}
	return i;
}

// função que divide cada comando nos seus argumentos
void parse_args(char *cmd, char *args[]) {
	int i = 0;
	char *token = strtok(cmd, " ");
	while(token != NULL) {
		args[i++] = token;
		token = strtok(NULL, " ");
	}
	args[i] = NULL;
}

void exec_pipelining(char *input) {
	int num_cmds;
	char *cmds[MAX_CMDS];
	int pipes[MAX_CMDS][2];

	num_cmds = split_cmds(input, cmds);

	for(int i = 0; i < num_cmds; i++) {
		if(i < num_cmds - 1) {
			if(pipe(pipes[i]) == -1) {
				perror("pipe");
				_exit(1);
			}
		}

		int pid = fork();
		if(pid < 0) {
			perror("fork");
			_exit(1);
		}

		if(pid == 0) { // processo filho
			if(i > 0) {
				dup2(pipes[i-1][0], STDIN_FILENO);
				close(pipes[i-1][0]);
				close(pipes[i-1][1]);
			}
			if(i < num_cmds - 1) {
				dup2(pipes[i][1], STDOUT_FILENO);
				close(pipes[i][0]);
				close(pipes[i][1]);
			}

			char *args[MAX_ARGS];
			parse_args(cmds[i], args);
			execvp(args[0], args);
			perror("execvp");
			_exit(1);
		} else {
			if(i > 0) {
				close(pipes[i-1][0]);
				close(pipes[i-1][1]);
			}
		}
	}

	for(int i = 0; i < num_cmds; i++) {
		wait(NULL);
	}
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
					printf("[processo_filho]: vou escrever no pipe\n");
					write(pipefd[1], &pack, sizeof(Package));
					close(pipefd[1]);
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
					printf("[proc. filho]Vou dar exec_command\n");
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
				} else if(pack.command_type == EXECUTE_MULT_COMMANDS) {
					close(pipefd[0]);
					write(pipefd[1], &pack, sizeof(Package));
					close(pipefd[1]);
					char clientFifo[32];
					snprintf(clientFifo, 32, "tmp/fifo%d", pid_client);
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
					exec_pipelining(pack.command);
					return EXIT_SUCCESS;
				}
			} else { // processo pai
				close(pipefd[1]);
				Package pack_buffer;
				int status;
				printf("[processo-pai] vou ler do pipe\n");
				read(pipefd[0], &pack_buffer, sizeof(Package));
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