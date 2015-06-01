/* O programa cliente envia pedidos através do pipe com nome criado pela cloudshell 
 * para a CloudShell e espera pelo resultado da operação.
 * A CloudShell já deverá estar a correr para o cliente funcionar, caso contrário
 * não conseguirá abrir os pipes de pedidos e respostas.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "readln.h"

#define LINE 128    /* Tamanho máximo da linha */

char * response;    /* Para guardar a resposta da CloudShell */
int rsp_p;          /* Descriptor para o pipe de respostas */
int nbytes;         /* Número de bytes lidos no read */

/* Função que vai tratar de ler a resposta enviada pelos processos,
 * isto permite que o cliente não bloqueie à espera de respostas
 * e consiga enviar mais pedidos enquanto os outros executam */
void sigcont_handler (int sig) { 
  printf("\n");
  while ((nbytes = readln(rsp_p, response, LINE)) > 0)
    printf("%s", response);
}

int main (int argc, char ** argv) {
  char * request;   /* Para guardar o pedido do cliente */
  char * pid;       /* Para guardar o PID do cliente */
  int req_p;        /* Descritor para o pipe de pedidos */

  signal(SIGCONT, sigcont_handler); /* Mudar função executada quando recebido SIGCONT */

  req_p = open("/tmp/csR", O_WRONLY); /* Abrir pipe para pedidos */
  rsp_p = open("/tmp/csA", O_RDONLY | O_NONBLOCK );   /* Abrir pipe para respostas */

  /* Verificar se houve falhas ao abrir os pipes */ 
  if ((req_p == -1) || (rsp_p == -1)) {
    printf("Não foi possivel ligar ao servidor.\n");
    return -1;
  }

  /* Alocar espaço para as strings */
  request   = (char *) malloc(sizeof(char) * LINE);
  response  = (char *) malloc(sizeof(char) * LINE);
  pid       = (char *) malloc(sizeof(char) * LINE);
  
  sprintf(pid, "%d\n", getpid());
  write(req_p, pid, strlen(pid));
  /* Ler o uma linha do std input */
  while (1) {
    write(1, "CShell$ ", strlen("CShell$ "));
    nbytes = readln(0, request, LINE);      /* Ler o comando */
    write(req_p, request, nbytes);          /* Enviar o comando */
  }
  return 0;
}
