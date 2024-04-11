#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int mysystem (const char* command) {

	int res = -1;

	char* exec_args[20];
	char* string, *tofree, *cmd;
	int i = 0, status;

	//copia string
	tofree = cmd = strdup(command);

	while((string = strsep(&cmd, " ")) != NULL){
		exec_args[i] = string;
		i++;
	} 

	exec_args[i] = NULL;

	pid_t pid = fork();

	switch(pid){
	case -1:{
		//código em caso de erro
		perror("erro no fork");
		res = -1;
		break;
	}
	case 0:{
		//código do processo filho
		//printf("FILHO | vou executar o %s\n", exec_args[0]);
		int res = execvp(exec_args[0], exec_args);
		_exit(res);
	}

	default:{
		//código do processo pai
		wait(&status);
		res = WEXITSTATUS(status);
		if(WIFEXITED(status)){
			printf("PAI | o filho %d devolveu %d\n", pid,res);
		}else{
			res = -1;
			printf("PAI | o filho não terminou corretamente\n");
		}
		break;
	}
	}

	free(tofree);


	return res;
}

void orchestrator(int N, char* commands[]){

	pid_t pid;
	int pids[N];
	int i, status, res;

	for(i = 0; i < N; i++){
		if((pid = fork()) == 0){
			execvp(commands[i], commands);
			_exit(i);
		} 
	}
	
	printf("PAI: Vou esperar pelos processos filho\n");

	for(int i = 0; i < N; i++){
		waitpid(pids[i], &status, NULL);
	}



	return 0;
}

int main(int argc, char *argv[])
{
	int i, N = 0;
	char *commands[argc-1];
	
	for(i = 0; i < argc; i++){
		commands[N] = strdup(argv[i]);
		N++;
	}

	orchestrator(N, *commands[N]);


	return 0;
}