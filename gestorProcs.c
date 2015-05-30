#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

pid_t pid;
int file;
int children;
int counter;
int i;
pid_t fpids[1024];
int active_procs;

void sigchld_handler (int sig) {
  int status, j;
  pid_t res = waitpid(-1, &status, 0);

  if (WIFEXITED(status)) {
    active_procs--;
    for (j = 0; j < children; j++)
      if (fpids[j] == pid) fpids[j] = -2;
  }
}

void sigalrm_handler (int sig) {
  kill(fpids[i], SIGSTOP);
  i++;
  if (i >= children) i = 0;

  while (fpids[i] == -2) {
    if (i >= children) i = 0; 
  }
  kill(fpids[i], SIGCONT);
  alarm(1);
}

void main(int argc, char ** argv) {
	children = argc - 1;
	pid_t pid = 1;
	active_procs = 0;
	mkfifo("/tmp/clienteR",0666);
	signal(SIGALRM, sigalrm_handler);
	signal(SIGCHLD, sigchld_handler);
	if((pid=fork())==0) {
		kill(getpid(), SIGSTOP);
		/*Abre o pipe para o cliente*/
		file = open("/tmp/clienteR", O_WRONLY);
		dup2(file,0); /*Impreme a execução para o pipe*/
    	
    	execlp(argv[i+1], argv[i+1], NULL);
	}
	else {
		fpids[i] = pid;
		active_procs++;
	}

	i = children - 1;
	kill(fpids[i], SIGCONT);
	alarm(1);

	while (counter <= children){
		pause();
    	/* Actualiza a estrutura */
    	if(i+1==children) break; /*condição para terminar o roundRobin*/
  	}
}