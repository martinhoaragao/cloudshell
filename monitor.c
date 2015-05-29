#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "readln.h"

#define LINE 128      /* Tamanho maximo da linha */

int main (int argc, char ** argv) {
  char * req;   /* Pedido */
  char * ans;   /* Resposta */
  int requests, answers;
  int nbytes;

  if ( mkfifo("/tmp/r", 066) == -1 ) {    /* Pipe com nome de pedidos (REQuests) */
    printf("Erro no pipe de pedidos.\n");
    _exit(-1);
  }
  if ( mkfifo("/tmp/a", 066) == -1 ) {   /* Pipe com nome de respotas (ANSwers) */ 
    printf("Erro no pipe de respostas.\n");
    _exit(-1);
  }

  requests  = open("/tmp/req", O_RDWR);
  answers   = open("/tmp/ans", O_RDWR); 
  
  while (1) {
    nbytes = readln(requests, req, LINE);
    write(1, req, nbytes);
  }

}
