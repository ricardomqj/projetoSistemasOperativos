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
    int expected_time;
    long timestamp;
    struct package *next;
} Package;

void receive_and_process_status(Package *head) {
    Package *current = head;
    int found_executed = 0, found_executing = 0, found_scheduled = 0;

    if(head == NULL) {
        printf("No packages to show status\n");
        return;
    }

    printf("Completed:\n");
    while (current != NULL) {
        if(current->status == EXECUTED && current->command_type != EXECUTE_STATUS) {
            printf("%d %s timestamp: %ld ms\n", current->id, current->command, current->timestamp);
            found_executed = 1;
        }
        current = current->next;
    }
    if (!found_executed) {
        printf("No programs executed\n");
    }

    current = head;  
    printf("Executing:\n");
    while (current != NULL  && current->command_type != EXECUTE_STATUS) {
        if(current->status == EXECUTING) {
            printf("%d %s\n", current->id, current->command);
            found_executing = 1;
        }
        current = current->next;
    }
    if (!found_executing) {
        printf("No programs executing\n");
    }

    current = head;  
    printf("Scheduled:\n");
    while (current != NULL  && current->command_type != EXECUTE_STATUS) {
        if(current->status == NOT_EXECUTED) {
            printf("%d %s\n", current->id, current->command);
            found_scheduled = 1;
        }
        current = current->next;
    }
    if (!found_scheduled) {
        printf("No programs not executed\n");
    }
}

int main(int argc, char *argv[]) {
	int fdFifoCliOrch = open("tmp/fifoCliOrch", O_WRONLY);
    pid_t pid;

	if(fdFifoCliOrch == -1) {
		perror("open");
		_exit(1);
	}

    if(strcmp(argv[1], "execute") == 0) {
        if(strcmp(argv[3], "-u") == 0) {
            int expected_time = atoi(argv[2]);
            pid = fork();
            if(pid == -1) {
                perror("fork");
                _exit(1);
            }

            if(pid == 0) { // código do processo filho
                pid_t pid_filho = getpid();
                char fifoName[32];
                sprintf(fifoName, "tmp/fifo%d", pid_filho);
                mkfifo(fifoName, 0666); 
                Package pack;
                strcpy(pack.command, argv[4]);
                pack.pid = pid_filho;
                pack.expected_time = expected_time;
                pack.id = -1;
                pack.command_type = EXECUTE_COMMAND;
                pack.expected_time = expected_time;
                pack.timestamp = -1;
                pack.next = NULL;
                if(write(fdFifoCliOrch, &pack, sizeof(Package)) < 0) {
                    perror("write");
                    _exit(1);
                }
                int fdFifoOrchCli = open(fifoName, O_RDONLY);
                if(fdFifoOrchCli == -1) {
                    perror("open");
                    _exit(1);
                } 
                int id_received_buffer;
                if(read(fdFifoOrchCli, &id_received_buffer, sizeof(int)) < 0) {
                    perror("read");
                    _exit(1);
                } 
                printf("TASK %d Received\n", id_received_buffer); 
            } 
        } else if(strcmp(argv[3], "-p") == 0) {
            pid = fork();
            int expected_time = atoi(argv[2]);
            if(pid == -1) {
                perror("fork");
                _exit(1);
            } else if(pid == 0) {
                pid_t pid_filho = getpid();
                char fifoName[32];
                sprintf(fifoName, "tmp/fifo%d", pid_filho);
                mkfifo(fifoName, 0666);
                Package pack;
                strcpy(pack.command, argv[4]);
                pack.pid = pid_filho;
                pack.id = -1;
                pack.command_type = EXECUTE_MULT_COMMANDS;
                pack.timestamp = -1;
                pack.expected_time = expected_time;
                pack.next = NULL;
                if(write(fdFifoCliOrch, &pack, sizeof(Package)) < 0) {
                    perror("write");
                    _exit(1);
                }   
                int fdFifoOrchCli = open(fifoName, O_RDONLY);
                if(fdFifoOrchCli == -1) {
                    perror("open");
                    _exit(1);
                }
                int id_received_buffer;
                if(read(fdFifoOrchCli, &id_received_buffer, sizeof(int)) < 0) {
                    perror("read");
                    _exit(1);
                }
                printf("TASK %d Received\n", id_received_buffer);
            }
        } else {
            printf("Invalid option. Use '-u' or '-p'...\n");
            return 0;
        }
    } else if(strcmp(argv[1], "status") == 0) {
        pid = fork();
        if(pid == -1) {
            perror("fork");
            _exit(1);
        } else if (pid == 0) {
            // código do processo filho
            Package pkg;
            Package *head = NULL, *tail = NULL, *new_pkg;
            pid_t pid_filho = getpid();
            char fifoName[32];
            sprintf(fifoName, "tmp/fifo%d", pid_filho);
            mkfifo(fifoName, 0666);
            Package pack;
            pack.id = -1;
            pack.command_type = EXECUTE_STATUS;
            pack.pid = pid_filho;
            pack.expected_time = -1;
            pack.timestamp = -1;
            strcpy(pack.command, "STATUS_COMMAND");
            pack.status = NOT_EXECUTED;
            if(write(fdFifoCliOrch, &pack, sizeof(Package)) < 0) {
                perror("write");
                _exit(1);
            }
            int fdFifoOrchCli = open(fifoName, O_RDONLY);
            while(read(fdFifoOrchCli, &pkg, sizeof(Package)) > 0) {
                new_pkg = malloc(sizeof(Package));
                *new_pkg = pkg;
                new_pkg->next = NULL;
                if(head == NULL) head = tail = new_pkg;
                else {
                    tail->next = new_pkg;
                    tail = new_pkg;
                }
            }
            receive_and_process_status(head);
            close(fdFifoOrchCli);
            unlink(fifoName);

        } 
    } else {
        printf("Comando inserido é inválido!\n");
        return 0;
    }
    close(fdFifoCliOrch);
    return 0;
}
