#include <stdlib.h>
#include <unistd.h>

size_t readln (int fildes, char * buf, size_t nbyte) {
  size_t bytesread = 0, i = 0;
  char c;

  do {
    i = 0;
    i = read(fildes, &c, 1);
    if (i != 0) { buf[bytesread] = c; bytesread++; }
  } while ( i != 0 && bytesread != nbyte && c != '\n');

  buf[bytesread] = '\0';

  // Devolver o numero de bytes lidos
  return bytesread;
}
