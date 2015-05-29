/* O programa cliente envia processos atraves do pipe partilhado criado pela cloudshell 
 * para a CloudShell e espera pelo resultado da operação */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "readln.h"

void sigcont_handler (int sig);

#define LINE 128

int main (int argc, char ** argv) {
  char * buff;      /* Para guardar o input */
  char * pid;       /* Para guardar o pid do processo */
  char * response;  /* Para guardar a respota da CloudShell */
  char * spid;      /* Para guardar o pid da CloudShell */
  int fd;           /* Descritor para o pipe anonimo */
  int nbytes;       /* Num. de bytes lidos no read */

  signal(SIGCONT, sigcont_handler);

  /* Abrir o pipe partilhado para leitura e escrita */
  fd = open("/tmp/cshell", O_RDWR);

  /* Falha ao abrir o pipe */
  if (fd == -1) {
    printf("Não foi possivel ligar ao servidor.\n");
    return -1;
  }

  /* Alocar espaço para as strings */
  buff = (char *) malloc(sizeof(char) * LINE);
  pid = (char *) malloc(sizeof(char) * LINE);
  response = (char *) malloc(sizeof(char) * LINE);
  spid = (char *) malloc(sizeof(char) * LINE);

  /* Guardar o pid do cliente como string */
  sprintf(pid, "%d\n", getpid());

  /* Ler o input do std input até encontrar end-of-file */
  while (1) {
    nbytes = read(0, buff, LINE);     /* Ler o comando */
    write(fd, pid, strlen(pid));  /* Enviar o pid */
    write(fd, buff, nbytes);          /* Enviar o comando */

    /* Esperar pelo sinal de que a resposta está pronta */
    pause();
    nbytes = read(fd, spid, LINE);
    spid[nbytes - 1] = '\0';
    kill(atoi(spid), SIGCONT);

    /* Adicionar codigo que coloca o cliente em pausa espera pelo envio do sinal 
     * SIGALRM do pai para ler o resultado de correr o processo que este cliente
     * pediu para correr */
  }

  close(fd);
  unlink("/tmp/cshell");

  return 0;
}

void sigcont_handler (int sig) {
  printf("Resposta recebida!\n");
}
