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

int main (int argc, char ** argv) {
  char * request;   /* Para guardar o pedido do cliente */
  char * response;  /* Para guardar a respota da CloudShell */
  int req_p;        /* Descritor para o pipe de pedidos */
  int rsp_p;        /* Descritor para o pipe de respostas */
  int nbytes;       /* Num. de bytes lidos no read */

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
