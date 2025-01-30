#ifndef NUMBERS_H
#define NUMBERS_H
/* Does the platform support "%'d" for thousands commas?
   - this was standardized in POSIX 2008 */
#include <unistd.h>
#include <locale.h>
#if (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L) || (defined(__DARWIN_C_LEVEL) && __DARWIN_C_LEVEL >= 200809L)
#define QUOT "'"
static inline void InitNumbers() {
  setlocale(LC_NUMERIC,""); }
#else
/* assume no support - leave it out */
#define QUOT ""
static inline void InitNumbers() {}
#endif
static inline void prnSeconds(long numSecs) {
  if(numSecs>=3600) fprintf(stderr," (%d:%02d:%02d)",(int)(numSecs/3600L),(int)(numSecs%3600L)/60,(int)(numSecs%60L));
  else {
    int secs = (int)numSecs;
    if(secs>=60) fprintf(stderr," (%dmin %dsec)",secs/60,secs%60);
    else if(secs>1) fprintf(stderr," (%d seconds)",secs);
    else fprintf(stderr," (1 second)");
  }
}
#endif
