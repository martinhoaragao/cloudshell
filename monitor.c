#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <bsd/string.h>
#include "readln.h"

#define LINE 128      /* Tamanho maximo da linha */

int main (int argc, char ** argv) {
  char * req;   /* Pedido */
  char * ans;   /* Resposta */
  char teste[1000];
  char commnd[100];
  int requests, answers;
  int nbytes;
  FILE *fp;
  char *ppid;






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

  while (1) {
    nbytes = readln(requests, req, LINE);
    write(1, req, nbytes);
  }

 /* Pid process */
  ppid='2';

  /* Build command to obtain CPU usage */
  strcpy (commnd, "pidstat -hu -p ");
  strncat(commnd, &var, 100);
  strncat(commnd, " | awk 'NR==4 {print $7}'", 100);
  printf("%s", commnd);

  fp = popen(commnd, "r");
  if (fp == 0)
    printf("Ficheiro inexistente!\n");
  else {
    while (fgets(teste, 100, fp) != 0);
  }
  pclose(fp);
  printf("%s", teste);

  /* Need time, and previous CPU usage to update credit */

}
/*

*/
