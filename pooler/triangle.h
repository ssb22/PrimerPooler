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
/* We define a 'triangle cache' as one whose elements
   represent every possible pairing in a symmetric way:
   0 paired with 0...n-1, 1 paired with 1...n-1, 2 with 2...
   etc.  Can halve the rectangle that we need to allocate */
static inline int t_Nitems(int n) {
  /* total number of items in an n*n triangle - this is
     just normal triangle-numbers theory */
  return n*(n+1)/2;
}

static inline int _t_offset(int n,int i,int j) {
  /* Offset of the (i,j) pair assuming j >= i */
  /* line 0 starts at 0
     line 1 starts at n
     line 2 starts at n + n-1  = 2n - 1
     line 3 starts at n + n-1 + n-2 = 3n - (1+2)
     ...
     line N starts at N*n - Tri(N-1)
  */
  // return n*i - (i-1)*i/2 + j-i;
  // which is
  return (n-1)*i - (i-1)*i/2 + j;
}
static inline int t_offset(int n,int i,int j) {
  if(j>=i) return _t_offset(n,i,j);
  else return _t_offset(n,j,i);
}

#include "openmp.h"
typedef struct { int start,end; } Range;
typedef struct { Range r[2]; } TwoRanges;
static inline TwoRanges t_iBounds(int n) {
  /* distribute bounds to evenly schedule among threads.
     Work units should look like:
     0..1 and n..n
     1..2 and n-1..n
     2..3 and n-2..n-1
     ...
     n/2..n/2+1 and n-(n/2)..n-(n/2)+1 (if !=) */
  TwoRanges t;
  int nThreads = omp_get_num_threads(),
    perThread = n/2/nThreads, /* NB it's rounded down */
    tNum = omp_get_thread_num();
  if(!perThread) { /* unlikely but we should cover it */ nThreads=n/2; if(tNum>=nThreads) { t.r[0].start=t.r[0].end=t.r[1].start=t.r[1].end=0; return t; } else perThread=1; }
  t.r[0].start = tNum*perThread;
  if(tNum==nThreads-1) {
    /* careful, due the above round-down */
    t.r[0].end = t.r[1].start = n/2; /* make SURE they meet in the middle and nothing gets left out */
  } else {
    t.r[0].end = (tNum+1)*perThread;
    t.r[1].start = n-(tNum+1)*perThread;
  }
  t.r[1].end = n-tNum*perThread;
  if(t.r[0].start==t.r[1].start) t.r[1].start=n;
  return t;
}
static inline time_t t_ProgressStart(const char *p) { fputs(p,stderr); fflush(stderr); return time(NULL)+2; }
static inline void t_Progress(const char *p,TwoRanges tr,int n,int done,time_t *next) {
  if(!omp_get_thread_num() /* only thread 0 prints */ && time(NULL) >= *next) {
    fprintf(stderr,"\r%s%d%%",p,100*done/(n*(tr.r[0].end-tr.r[0].start))); fflush(stderr);
    *next = time(NULL) + 2;
  }
}
