/* Este programa server como servidor, vai criar o fifo e depois lê o que os clientes escrevem
 * no fifo, executam os processos pretendidos e escreve o resultado na bash dos clientes */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "readln.h"
#include <sys/shm.h>

size_t readln (int, char *, size_t);
void sigalrm_handler (int);
void sigchld_handler (int);
void gestProcs ();
void roundRobin ();
void switchProcs (int);

#define LINE 128  /* Tamanho para a linha de input */
int fd;           /* Descritor para o pipe partilhado */
pid_t proc_pid;   /* PID do gestor de processos */
int proc_p[2];    /* Pipe para usar com o gestor de processos */
pid_t rr_pid;     /* PID do processo que executa roundRobin */
pid_t * procs;    /* Array dos processos a correr */
int * active;     /* Número de processos activos */
int * actual;     /* Indice do processo actual */
int * c_pid;      /* PID do cliente */
int shmid;

int main (int argc, char ** argv) {
  char * req;         /* Pedido do cliente */
  char * cpid;        /* String com o PID do cliente */
  char * token;       /* Auxiliar para separar argumentos */
  int nbytes;         /* Numero de bytes lidos no read */
  int i;              /* Iterador auxiliar */
  int req_p;          /* Pipe para ler pedidos do cliente */

  /* Mudar função de resposta a sinais */
  signal(SIGALRM, sigalrm_handler);

  /* Alocar memoria para as strings */
  req   = (char *) malloc(sizeof(char) * LINE);
  cpid  = (char *) malloc(sizeof(char) * LINE);

  /* Preparar o array de PIDS */
  shmid = shmget(IPC_PRIVATE, sizeof(pid_t) * 100, SHM_W | SHM_R | IPC_CREAT ); 
  procs = (pid_t *) shmat(shmid, NULL, 0);
  /* Preparar o número de processos activos */
  shmid = shmget(IPC_PRIVATE, sizeof(int), SHM_W | SHM_R | IPC_CREAT );
  active = (int *) shmat(shmid, NULL, 0);
  /* Preparar o indice do processo actual */
  shmid = shmget(IPC_PRIVATE, sizeof(int), SHM_W | SHM_R | IPC_CREAT );
  actual = (int *) shmat(shmid, NULL, 0);
  /* Preparar o PID do cliente */
  shmid = shmget(IPC_PRIVATE, sizeof(int), SHM_W | SHM_R | IPC_CREAT );
  c_pid  = (int *) shmat(shmid, NULL, 0);

  /* Criar processo que vai gerir recursos */
  pipe(proc_p);
  proc_pid = fork();
  if (proc_pid == 0) { gestProcs(); exit(0); }
  else if (proc_pid == -1) {
    printf("Erro a criar Gestor de Recursos\n");
    exit(-1);
  }
  close(proc_p[0]);

  /* Criar processo responsavel pelo roundRobin */
  rr_pid = fork();
  if (rr_pid == 0) { roundRobin(); exit(0); }
  else if (rr_pid == -1) {
    printf("Erro ao criar processo de roundRobin\n");
    exit(-1);
  }

  /* Criar pipe para ler pedidos do cliente */
  mkfifo("/tmp/csR", 0666);
  req_p = open("/tmp/csR", O_RDONLY);
  /* Criar pipe para escrever respostas para o cliente */ 
  mkfifo("/tmp/csA", 0666);
  nbytes = open("/tmp/csA", O_RDWR);
  close(nbytes);

  /* Ler o pid do cliente */
  nbytes = read(req_p, cpid, LINE);
  *c_pid = atoi(cpid);

  /* Começar a leitura de argumentos */
  alarm(1);
  while (1) {
    nbytes = readln(req_p, req, LINE);    /* Ler pedido do cliente */
    write(proc_p[1], req, nbytes);
  }

  return 0;
}

/* Função que verifica se os gestores estão todos a correr e caso algum
 * não esteja, a CloudShell volta a cria-lo */
void sigalrm_handler (int sig) {
  int status;   /* Guardar o estado do filho */
  pid_t aux;    /* PID Auxiliar */

  if ((aux = waitpid(proc_pid, &status, WNOHANG)) == proc_pid)
    if ((proc_pid = fork()) == 0) { gestProcs(); exit(0); }

  if ((aux = waitpid(rr_pid, &status, WNOHANG)) == rr_pid)
    if ((rr_pid = fork()) == 0) { roundRobin(); exit(0); }

  alarm(1);
}

///////////////////////////////  GESTOR DE PROCESSOS   ///////////////////////////////////

void gestProcs () {
  int i;            /* Iterador Auxiliar */
  int resp;         /* Descritor para respostas */
  char * args[16];  /* Argumentos */
  char * req;       /* Pedido do cliente */
  pid_t cpid;       /* PID do filho criado */

  signal(SIGCHLD, sigchld_handler);

  *active = 0;
  close(proc_p[1]);
  req   = (char *) malloc(sizeof(char) * LINE);

  while (1) {
    i = 0;
    read(proc_p[0], req, LINE);

    args[i] = strtok(req, " \n");
    while (args[i]) {
      args[++i] = strtok(NULL, " \n");
    }

    if ((cpid = fork()) == 0) { /* Filho para correr o comando */
      kill(getpid(), SIGSTOP);  /* Filho para-se a si mesmo */
      resp = open("/tmp/csA", O_WRONLY);
      dup2(resp, 1); close(resp);
      execvp(args[0], args);
      exit(0);
    } else {
      (*active)++;
      /* Inserir pid do filho no array */
      for (i = 0; procs[i] != 0; i++) ;
      printf("<< %d - %d\n", i, cpid);
      procs[i] = cpid; 
    }
  }
}

void sigchld_handler (int sig) {
  int status;   /* Estado do filho */
  pid_t pid;    /* PID auxiliar */
  int i, j;     /* Iteradores auxiliares */

  if ((pid = waitpid(-1, &status, WNOHANG)) != 0) {
    if (WIFEXITED(status)) {
      for (i = 0; procs[i] && (procs[i] != pid); i++) ;
      for (j = i; j < *active; j++) procs[j] = procs[j+1]; 
      (*active)--;
      kill(*c_pid, SIGCONT);
    }
  }
}

//////////////////////////////   ROUND ROBIN   ///////////////////////////////////////////

void roundRobin () {
  signal(SIGALRM, switchProcs);
  *actual = 0;

  alarm(1);
  while (1) {
    pause();
  }
}

/* Aplica roundRobin aos processos que estão actualmente no array de processos */
void switchProcs (int sig) {
  if (*active > 0) {
    if (procs[*actual]) kill(procs[*actual], SIGSTOP);
    (*actual)++;
    if (*actual >= *active) *actual = 0;
    if (procs[*actual]) kill(procs[*actual], SIGCONT);
  }
  alarm(1);
}
