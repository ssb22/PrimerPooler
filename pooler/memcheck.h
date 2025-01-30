int _memFail(const char *file,int line,void *first,...); /* declared wrongly before v1.86, causing spurious 'out of memory' errors if compiled on M1 Mac, other platforms not affected */
#define memFail(...) _memFail(__FILE__, __LINE__, __VA_ARGS__)
static inline void* memTrim(void *p,void *top) {
  void *r = realloc(p,top-p);
  return r ? r : p;
}
