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
#include <time.h>

size_t readln (int, char *, size_t);
void sigalrm_handler (int);
void sigchld_handler (int);
void gestProcs ();
void roundRobin ();
void monitor();
void switchProcs (int);
void updateStats (int);

#define LINE 128  /* Tamanho para a linha de input */
int fd;           /* Descritor para o pipe partilhado */
pid_t proc_pid;   /* PID do gestor de processos */
int proc_p[2];    /* Pipe para usar com o gestor de processos */
pid_t rr_pid;     /* PID do processo que executa roundRobin */
int * active;     /* Número de processos activos */
int * actual;     /* Indice do processo actual */
int * c_pid;      /* PID do cliente */
int shmid;
pid_t * procs;    /* Array dos processos a correr */
double * cpu;     /* Array com a utilização de CPU de cada processo */
int m_r, m_a;     /* Descritores para comunicação com a monitorização */
pid_t m_pid;      /* PID da monitorização */

int main (int argc, char ** argv) {
  char * req;         /* Pedido do cliente */
  char * cpid;        /* String com o PID do cliente */
  char * token;       /* Auxiliar para separar argumentos */
  char * m_ans;       /* Para guardar a resposta da monitorização */
  int nbytes_1;       /* Numero de bytes lidos no read */
  int nbytes_2;
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
  /* Prepara o array de CPU */
  shmid = shmget(IPC_PRIVATE, sizeof(double), SHM_W | SHM_R | IPC_CREAT );
  cpu = (double *) shmat(shmid, NULL, 0);
  /* Prepara string de receção de resposta da Monitorização */
  m_ans = (char *) malloc(sizeof(char) * LINE);

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

  /* Criar processo responsavel pela monitorização */
  m_pid = fork();
  if (m_pid == 0) { monitor(); exit(0); }
  else if (m_pid == -1) {
    printf("Erro ao criar processo de Monitorização\n");
    exit(-1);
  }

  /* Criação de pipes com nomes */
  mkfifo("/tmp/csR", 0666); /* Pipes para pedidos de clientes */
  mkfifo("/tmp/csA", 0666); /* Pipes para respostas ao cliente */
  mkfifo("/tmp/m_r", 0666); /* Pipe para pedidos à monitorização */
  mkfifo("/tmp/m_a", 0666); /* Pipe para respostas da monitorização */

  /* Abertura de descritores */
  req_p = open("/tmp/csR", O_RDONLY);
  m_r   = open("/tmp/m_r", O_WRONLY);
  m_a   = open("/tmp/m_a", O_RDONLY | O_NONBLOCK);

  /* Ler o pid do cliente */
  nbytes_1 = read(req_p, cpid, LINE);
  *c_pid = atoi(cpid);
  printf("%d\n", *c_pid);

  /* Começar a leitura de argumentos */
  alarm(1);
  while (1) {
    nbytes_1 = readln(req_p, req, LINE);    /* Ler pedido do cliente */
    write(m_r, cpid, strlen(cpid));
    nbytes_2 = readln(m_a, m_ans, LINE);
    if (strcmp(m_ans, "OK\n") == 0)
      write(proc_p[1], req, nbytes_1);
    else if (strcmp(m_ans, "KO\n") == 0)
      write(proc_p[1], "terminate\n", strlen("terminate\n"));
    else
      write(proc_p[1], req, nbytes_1);
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

  if ((aux = waitpid(m_pid, &status, WNOHANG)) == m_pid)
    if ((m_pid = fork()) == 0) { monitor(); exit(0); }

  alarm(1);
}

///////////////////////////////  GESTOR DE PROCESSOS   ///////////////////////////////////

void terminate () { /* Terminar todos os processos criados pelo cliente */
  int i;
  for (i = 0; i < *active; i++)
    kill(procs[i], SIGKILL);
  *active = 0;
  exit(0);
}

void gestProcs () {
  int i;            /* Iterador Auxiliar */
  int resp;         /* Descritor para respostas */
  char * args[16];  /* Argumentos */
  char * req;       /* Pedido do cliente */
  pid_t cpid;       /* PID do filho criado */
  char * temp;

  signal(SIGCHLD, sigchld_handler);

  *active = 0;
  close(proc_p[1]);
  req   = (char *) malloc(sizeof(char) * LINE);

  while (1) {
    i = 0;
    read(proc_p[0], req, LINE);

    if (strcmp(req, "terminate\n") == 0)  /* Matar todos os processos */
      terminate();

    args[i] = strtok(req, " \n");
    while (args[i]) args[++i] = strtok(NULL, " \n");

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
      procs[i]    = cpid; 
      cpu[i]      = 0.0;
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
      for (j = i; j < *active; j++) {
        procs[j]  = procs[j+1];
        cpu[j]    = cpu[j+1];
      }
      (*active)--;
      kill(*c_pid, SIGCONT);
    }
  }
}

//////////////////////////////   ROUND ROBIN      ///////////////////////////////////////////

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

///////////////////////////     MONITORIZAÇÃO     //////////////////////////////////

double credit_max, credit_now;

void monitor () {
  int reqs, ans;    /* Descritores de pedidos e respostas */
  char * m_request; /* Para guardar a string do pedido */
  int nbytes;
  char c;
  signal(SIGALRM, updateStats);
  credit_max = 20.0;

  reqs      = open("/tmp/m_r", O_RDONLY);
  ans       = open("/tmp/m_a", O_WRONLY);
  m_request = (char *) malloc(sizeof(char) * LINE);
  
  alarm(1);
  while (1) {
    nbytes = read(reqs, m_request, LINE);
    if (credit_now < 0.0)
      write(ans, "KO\n", strlen("KO\n"));
    else
      write(ans, "OK\n", strlen("OK\n"));
  }
}

void updateStats (int sig) {
  char cpu_total[100];
  char commnd[100];
  FILE *fp;
  double time_interval;
  int process;
  int i;                /* Iterador */

  for (i = 0; i < *active; i++) {
    process = (int) procs[i];
    /* Build command to obtain CPU usage */
    strcpy (commnd, "pidstat -hu -p ");
    sprintf(commnd, "%s %d | awk 'NR==4 {print $7}'", commnd, procs[i]);
    
    fp = popen(commnd, "r");
    if (fp == 0)
      printf("Ficheiro inexistente!\n");
    else {
      while (fgets(cpu_total, 100, fp) != 0);
    }
    pclose(fp);

    cpu[i] += atof(cpu_total);
    credit_now =  credit_max - (cpu[i]); 
  }
  alarm(1);
}

