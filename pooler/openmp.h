/*
# This file is part of Primer Pooler v1.61 (c) 2016-19 Silas S. Brown.  For Wen.
# 
# This program is free software; you can redistribute and
# modify it under the terms of the General Public License
# as published by the Free Software Foundation; either
# version 3 of the License, or any later version.
#
# This program is distributed in the hope that it will be
# useful, but WITHOUT ANY WARRANTY.  See the GNU General
# Public License for more details.
*/
#ifndef OPENMP_H
#define OPENMP_H
#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_max_threads() 1
#define omp_get_num_threads() 1
#define omp_get_thread_num() 0
#define omp_set_num_threads(x) (void)0
#endif

static inline void* wrapped_memcpy(void *a,const void *b,size_t n) {
  /* work around Apple compiler bug in Mac OS 10.7:
     if memcpy is called from a function that's doing
     OpenMP stuff, get linker errors.  So wrap it (but
     wrapping it in an INLINE function seems to work!) */
  return memcpy(a,b,n);
}

#include "random.h"
static inline int ThreadRand() {
  int tNum = omp_get_thread_num(); /* this might not be unique if ThreadRand is called from inside NESTED parallelism, but we don't do that */
  if(!tNum) return rand();
  static RandState *states = NULL;
  if (!states) {
    #ifdef _OPENMP
    #pragma omp critical
    #endif
    if (!states) {
      /* How many threads shall we leave room for?

         omp_get_max_threads() works BEFORE you start a
         parallel region, but not INSIDE it (it returns 1
         or however many threads you can NOW start).
         
         omp_get_num_procs() might not return a high
         enough number if someone set more threads than
         cores (not great for this application though).
         
         omp_get_num_threads() does what we want, as long
         as the parallel region has definitely started and
         we won't increase it later.  We've already
         established that we're in the parallel region
         via the above tNum test, so let's use that.
      */
      int nStates = omp_get_num_threads()-1;
      states = malloc(sizeof(RandState)*nStates);
      if(states) {
        int r = rand(), i;
        for(i=0; i<nStates; i++) states[i] = r++;
      }
    }
    if(!states) /* aaaargh! */ return rand();
  } return rand_r(states+(tNum-1));
}
#endif
