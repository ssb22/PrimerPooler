/*
This file is part of Primer Pooler (c) Silas S. Brown.  For Wen.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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
