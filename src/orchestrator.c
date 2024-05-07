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
#define MAX_ARGS 10
#define MAX_CMDS 5

typedef struct package {
    int id;
	char command[MAX_SIZE + 1];
	pid_t pid;
	int status;
	int command_type;
	int expected_time;
	long timestamp;
	struct package *next;
} Package;

typedef struct client {
	pid_t client_pid;
	Package *packages;
	struct client *next;
} Client;


void update_package_status(Client **head, int status, int id, long timestamp) {
	Client *current = *head;
	int updated = 0;
	while(current != NULL && !updated) {
		Package *currentPackage = current->packages;
		while(currentPackage != NULL && !updated) {
			if(currentPackage->id == id) {
				updated = 1;
				currentPackage->status = status;
				currentPackage->timestamp = timestamp;
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
			new_package->expected_time = pack.expected_time;
			new_package->timestamp = pack.timestamp;
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
	new_package->expected_time = pack.expected_time;
	new_package->timestamp = pack.timestamp;
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
			printf("Expected Time: %d\n", current_package->expected_time);
			printf("Timestamp: %ld\n", current_package->timestamp);
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
		Package *current_package = current_client->packages;
		while(current_package != NULL) {
			write(fdFifoOrchCli, current_package, sizeof(Package));
			current_package = current_package->next;
		}
		current_client = current_client->next;
	}
	close(fdFifoOrchCli);
}

void exec_command(int N, char* command, int task_id){
	pid_t pid;
	printf("Vou executar o comando %s\n", command);
    printf("task_id -> %d\n", task_id);
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
    for(int k = 0; k < j; k++){
        printf("cmd[%d] -> %s\n", k, cmd[k]);
    }

    char output_filename[64];
    snprintf(output_filename, 64, "output_folder/out%d.txt", task_id);
    printf("filename -> %s\n", output_filename);
	int fdOut = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if(fdOut == -1){
        perror("open");
        _exit(1);
    }

    if(dup2(fdOut, STDOUT_FILENO) == -1 || dup2(fdOut, STDERR_FILENO) == -1){
        perror("dup2");
        _exit(1);
    }
    close(fdOut);

	execvp(cmd[0], cmd);
	perror("execvp");
	_exit(1);
}


void parse_and_exec(char *cmd, int in_fd, int out_fd) {
    char *args[10];  
    int i = 0;

    while (*cmd && i < 10) {
        while (*cmd == ' ') cmd++; 

        if (*cmd == '\'') {  
            cmd++;
            args[i++] = cmd;
            while (*cmd && *cmd != '\'') cmd++;
            if (*cmd) *cmd++ = '\0';
        } else {
            args[i++] = cmd;
            while (*cmd && *cmd != ' ') cmd++;
            if (*cmd) *cmd++ = '\0';
        }
    }
    args[i] = NULL;

    if (in_fd != STDIN_FILENO) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    if (out_fd != STDOUT_FILENO) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    execvp(args[0], args);
    perror("execvp");
    _exit(1);
}

void exec_pipelining(char *command, int task_id) {
    int num_pipes = 0;
    char *cmd = command;
    char *tmp;

    while ((tmp = strchr(cmd, '|'))) {
        num_pipes++;
        cmd = tmp + 1;
    }

    int fd[2], in_fd = 0;
    int final_out_fd;

    cmd = command;
    for (int i = 0; i <= num_pipes; ++i) {
        if (i != num_pipes) {
            if (pipe(fd) == -1) {
                perror("pipe");
                _exit(1);
            }
        } else {
            char filename[256];
            snprintf(filename, sizeof(filename), "output_folder/out%d.txt", task_id);
            final_out_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (final_out_fd == -1) {
                perror("open file");
                _exit(1);
            }
        }

        tmp = strchr(cmd, '|');
        if (tmp) *tmp = '\0';

        int pid = fork();
        if (pid == -1) {
            perror("fork");
            _exit(1);
        } else if (pid == 0) {
            if (i != num_pipes) {
                close(fd[0]);
                parse_and_exec(cmd, in_fd, fd[1]);
            } else {
                parse_and_exec(cmd, in_fd, final_out_fd);
            }
        }

        if (in_fd != 0) close(in_fd);
        if (i != num_pipes) {
            close(fd[1]);
            in_fd = fd[0];
        } else {
            close(final_out_fd);
        }

        cmd = tmp + 1;
    }

    while (wait(NULL) > 0);
}

int main(int argc, char *argv[])
{
	struct timeval start, end;
	int N = 0;
	int id = 0; // variável usada para associar um id aos clientes
	pid_t pid;

	Client *client_list = NULL;

	if(mkfifo("tmp/fifoCliOrch", 0666) == -1) {
		perror("mkfifo");
		_exit(1);
	}

	int fdFifoCliOrch = open("tmp/fifoCliOrch", O_RDONLY);
	/*
    int fdOut = open("tmp/out.txt", O_WRONLY| O_CREAT | O_TRUNC, 0644);
	if(fdOut == -1){
		perror("open");
		_exit(1);
	} */
	while(1) {
		ssize_t bytes_read;
		pid_t client_pid;
		Package pack;
		bytes_read = read(fdFifoCliOrch, &pack, sizeof(Package));
		if(bytes_read > 0) {
			pid_t pid_client = pack.pid;
			id++;
			gettimeofday(&start, NULL);
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
					/*
                    if(dup2(fdOut,1) == -1){
						perror("dup2");
						_exit(1);
					}
					close(fdOut); */
					exec_command(N, pack.command, id);
					return EXIT_SUCCESS;
				} else if(pack.command_type == EXECUTE_STATUS) {
					char clientFifo[32];
					sprintf(clientFifo, "tmp/fifo%d", pid_client);
					int fdFifoOrchCli = open(clientFifo, O_WRONLY);
					if(fdFifoOrchCli == -1) {
						perror("open");
						_exit(1);
					}
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

					exec_pipelining(pack.command, id);
					return EXIT_SUCCESS;
				}
			} else { // processo pai
				close(pipefd[1]);
				Package pack_buffer;
				int status;
				read(pipefd[0], &pack_buffer, sizeof(Package));
				close(pipefd[0]);
				add_package(&client_list, pack_buffer, id);
				waitpid(pid, &status, 0);
				if(WIFEXITED(status) && WEXITSTATUS(status) == 0) {
					gettimeofday(&end, NULL);
					long seconds = (end.tv_sec - start.tv_sec); 
					long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec); 
					long millis = micros / 1000; 
					update_package_status(&client_list, EXECUTED, id, millis);
				
				} else {
					update_package_status(&client_list, EXECUTED_ERROR, id, 0);
				}
				close(pipefd[0]);
			}
		}
	}
	unlink("tmp/fifoCliOrch");
	return 0;
}
