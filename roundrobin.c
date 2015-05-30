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

void handle_sigchld(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
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

  signal(SIGALRM, sigalrm_handler);

  for (i = 0; (i < children) && pid; i++) {
    pid = fork();
    if (pid) {
      fpids[i] = pid;
    } else {
      kill(getpid(), SIGSTOP);
      execlp(argv[i+1], argv[i+1], NULL);
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