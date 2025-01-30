#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
int _memFail(const char *file,int line,void *first,...) {
  /* For use after doing a bunch of malloc()s.
     If any of the supplied args are NULL,
     write a message on standard error,
     free all non-NULL ones, and return 1.
     The address of _memFail itself is the terminator.
  */
  void *p = first; int all_OK = 1;
  va_list ap; va_start(ap, first);
  while(all_OK && p != _memFail) {
    if (p==NULL) all_OK = 0;
    p = va_arg(ap,void*);
  }
  va_end(ap);
  if(all_OK) return 0;
  fprintf(stderr,"Out of memory! (%s line %d)\n",file,line);
  p = first; va_start(ap, first);
  while(p != _memFail) {
    if(p) free(p);
    p = va_arg(ap,void*);
  }
  va_end(ap); return 1;
}
