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

size_t readln (int, char *, size_t);
void sigquit_handler (int);
void sigcont_handler (int);

#define LINE 128  /* Tamanho para a linha de input */
int fd;           /* Descritor para o pipe partilhado */

int main (int argc, char ** argv) {
  char * req;         /* Pedido do cliente */
  char * cpid;        /* Process id do cliente */
  char * args[16];    /* Argumentos do pedido */
  char * token;       /* Auxiliar para separar argumentos */
  char * pid;         /* Process id da CloudShell */
  int nbytes[2];      /* Numero de bytes lidos no read */
  int i;              /* Iterador auxiliar */

  /* Mudar função de resposta a sinais */
  signal(SIGCONT, sigcont_handler);
  signal(SIGQUIT, sigquit_handler);

  /* Criar o pipe partilhado */
  mkfifo("/tmp/cshell", 0666 );

  /* Abrir o pipe para leitura e escrita */
  fd = open("/tmp/cshell", O_RDWR);

  /* Alocar memoria para as strings */
  req   = (char *) malloc(sizeof(char) * LINE);
  cpid  = (char *) malloc(sizeof(char) * LINE);
  pid   = (char *) malloc(sizeof(char) * LINE);
  for (i = 0; i < 16; i++)
    args[i] = (char *) malloc(sizeof(char) * LINE);

  /* Registar o pid da CloudShell */
  sprintf(pid, "%d\n", getpid());

  while (1) {
    nbytes[0] = readln(fd, cpid, LINE);   /* Ler o pid do cliente */
    nbytes[1] = readln(fd, req,  LINE);   /* Ler o comando do cliente */

    /* Retirar '\n' das strings enviadas pelo cliente */
    cpid[strlen(cpid) - 1] = '\0';
    req[strlen(req)   - 1] = '\0';

    /* Criar o array dos argumentos */
    token = strtok(req, " ");
    for (i = 0; token != NULL; i++) {
      args[i] = token;
      token = strtok(NULL, " ");
    }
    args[i] = NULL;

    /* Criar processo para executar o pedido */
    if (fork() == 0) {
      execvp(args[0], args);
      _exit(0);
    } else {
      wait(NULL);                   /* Esperar pelo termino do filho */
      write(fd, pid, strlen(pid));  /* Escrever o pid da cloudshell no pipe */
      kill(atoi(cpid), SIGCONT);    /* Enviar sinal ao cliente para continuar */
      pause();                      /* Esperar por sinal do cliente */
    }
  }

  return 0;
}

/* Handler para o sinal SIGQUIT (Ctrl-\) que vai terminar o servidor */
void sigquit_handler (int sig) {
  close(fd);
  unlink("/tmp/cshell");
  _exit(0);
}

/* Handler para o sinal SIGCONT que devera ser enviado pelo cliente depois
 * de ler a respota enviada pela CloudShell */
void sigcont_handler (int sig) {
  printf("Sinal recebido!\n");
}
