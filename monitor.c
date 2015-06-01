#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include "readln.h"

#define LINE 128      /* Tamanho maximo da linha */

/*
typedef struct proc_monitor {
	int pid;
  clock_t tempo;
  double cpu_usage;
	struct Proc_Monitor *ants;
	struct Proc_Monitor *prox;
} *Proc_Monitor;
*/

/* list */
/* credit_max */
/* credit_now */

void main (int argc, char ** argv) {
  char * req;   /* Pedido */
  char * ans;   /* Resposta */
  int requests, answers;
  int nbytes;
  clock_t start, stop;

  signal(SIGALRM, monitor);


  if ( mkfifo("/tmp/req", 066) == -1 ) {    /* Pipe com nome de pedidos (REQuests) */
    printf("Erro no pipe de pedidos.\n");
    _exit(-1);
  }
  if ( mkfifo("/tmp/ans", 066) == -1 ) {   /* Pipe com nome de respotas (ANSwers) */
    printf("Erro no pipe de respostas.\n");
    _exit(-1);
  }

  requests  = open("/tmp/req", O_RDWR);
  answers   = open("/tmp/ans", O_RDWR);


  while(1){
    alarm(1);

    while (1) {
      nbytes = readln(requests, req, LINE);
      write(requests, req, nbytes);
    }
    if(req != 0) {
      monitor(0);
      write(answers, itoa(credit_now), sizeof(int)+1);
    }

  }

  /* Need time, and previous CPU usage to update credit */
}


int monitor (int sig) {
  Proc_Monitor process;
  char cpu_total[100];
  char commnd[100];
  FILE *fp;
  clock_t now;
  double time_interval;

  process = active_processes;  /*Update list according to global variable name */

  while(process) {
    /* Build command to obtain CPU usage */
    strcpy (commnd, "pidstat -hu -p ");
    strncat(commnd, itoa (active_processes->pid), 100);
    strncat(commnd, " | awk 'NR==4 {print $7}'", 100);

    fp = popen(commnd, "r");
    if (fp == 0)
      printf("Ficheiro inexistente!\n");
    else {
      while (fgets(cpu_total, 100, fp) != 0);
    }
    pclose(fp);
    printf("%s", cpu_total);

    now = clock();
    time_interval = ((double)stop-start)/CLOCKS_PER_SEC;
    active_processes->cpu_usage += atof(cpu_total);
    credit_now =  credit_max - ((active_processes->cpu_usage) * time_interval); /* Used 50% cpu across 10 minutes, that's 500 credits. Same as 500% cpu in 1 minute. So Credits are cpu/min */
    process = active_processes->prox;
  }

}
