#include <stdio.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

int children;
int counter;
int i;
pid_t fpids[1024];  /* Array de pids */

void sigchld_handler (int signo) {
	pid_t res;
	int status;
	while((res = waitpid(-1, &status, WNOHANG))>0) {
		if(WIFEXITED(status)) {
			printf("o processo %d terminou normalmente (%d)\n", res, WEXITSTATUS(status) );	
		}
		else {
			printf("O processo %d não terminou normalmente\n",res);
		}
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

void roundRobin (int argc, char ** argv) {
  children = argc - 1;
  pid_t pid = 1;

  //signal(SIGCHLD, sigchld_handler);  ***Not working with SIGCHLD***
  signal(SIGALRM, sigalrm_handler);
  signal(SIGCHLD, sigchld_handler);

  for (i = 0; (i < children) && pid; i++) {
    pid = fork();
    if (pid) {
      fpids[i] = pid;
    } else {
      kill(getpid(), SIGSTOP);
      execlp(argv[i+1], argv[i+1], NULL);
      signal(SIGCHLD, sigchld_handler);
    }
  }

  i = children - 1;
  kill(fpids[i], SIGCONT);
  alarm(1);

  while (counter <= children){
    pause();
    if(i+1==children) break; /*condição para terminar o roundRobin*/
  }
}




/*Só criei este main para ver se consigo executar o roundRobin com outros programas*/
void main(int argc, char ** argv) {
	if(fork()==0) {
		roundRobin(argc,argv);
	}
	else {
		wait(0L);
		printf("\n - - - FUCKING GENIUS - - - \n");
	}
}