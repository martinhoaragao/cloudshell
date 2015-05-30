/* O programa cliente envia pedidos através do pipe com nome criado pela cloudshell 
 * para a CloudShell e espera pelo resultado da operação.
 * A CloudShell já deverá estar a correr para o cliente funcionar, caso contrário
 * nºao conseguirá abrir os pipes de pedidos e respostas.
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
int rsp_p;          /* Descritor para o pipe de respostas */
int nbytes;         /* Número de bytes lidos no read */

/* Função que vai tratar de ler a resposta enviada pelos processos,
 * isto permite que o cliente não bloqueie à espera de respostas
 * e consiga enviar mais pedidos enquanto os outros executam */
void sigcont_handler (int sig) {
  while ((nbytes = readln(rsp_p, response, LINE)) > 0)
    write(1, response,nbytes);
}

int main (int argc, char ** argv) {
  char * request;   /* Para guardar o pedido do cliente */
  int req_p;        /* Descritor para o pipe de pedidos */

  signal(SIGCONT, sigcont_handler); /* Mudar função executada quando recebido SIGCONT */

  req_p = open("/tmp/csR", O_RDWR); /* Abrir pipe para pedidos */
  rsp_p = open("/tmp/csA", O_RDWR); /* Abrir pipe para respostas */

  /* Verificar se houve falhas ao abrir os pipes */ 
  if ((req_p == -1) || (rsp_p == -1)) {
    printf("Não foi possivel ligar ao servidor.\n");
    return -1;
  }

  /* Alocar espaço para as strings */
  request   = (char *) malloc(sizeof(char) * LINE);
  response  = (char *) malloc(sizeof(char) * LINE);
  
  /* Ler o uma linha do std input */
  while (1) {
    nbytes = readln(0, request, LINE);      /* Ler o comando */
    write(req_p, request, nbytes);          /* Enviar o comando */

    nbytes = readln(rsp_p, response, LINE);  /* Esperar por resposta da cloudshell */
    write(1, response, nbytes);             /* Imprimir a resposta no std out */
  }
  return 0;
}
