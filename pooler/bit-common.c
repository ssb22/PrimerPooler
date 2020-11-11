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
/* bit-common.c is the only file that's allowed to include
   bit-choice.h, expanding to the architecture-dependent
   choice of (32 + 64) or (64 + 128) bit functions.
   All other modules that need these functions should
   include all-primers.h instead.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "deltaG-degen.h"
#include "memcheck.h"
#include "numbers.h"
static void indent(int n,FILE*f){while(n--)fputc(' ',f);}
static const char degenerateCombos[]="TGKCYSBAWRDMHVN";
static void reportUnrecognisedBase(unsigned char b);
typedef struct { int score, i, j; } ScoreRecord;
typedef struct { float dG; int i, j; } DG_ScoreRecord;
static int highestScore1st(const void * a, const void *b) {
  /* for printBonds: print highest-scoring 1st, else keep input order */
  int r = ((ScoreRecord*)b)->score - ((ScoreRecord*)a)->score; /* <0 if A higher */
  if(r) return r;
  r = ((ScoreRecord*)a)->i - ((ScoreRecord*)b)->i; /* <0 if A lower */
  return r ? r : (((ScoreRecord*)a)->j - ((ScoreRecord*)b)->j);
}
static int dGhighestScore1st(const void * a, const void *b) {
  if(((DG_ScoreRecord*)a)->dG < ((DG_ScoreRecord*)b)->dG) return -1;
  else if(((DG_ScoreRecord*)a)->dG > ((DG_ScoreRecord*)b)->dG) return 1;
  int r = ((DG_ScoreRecord*)a)->i - ((DG_ScoreRecord*)b)->i;
  return r ? r : (((DG_ScoreRecord*)a)->j - ((DG_ScoreRecord*)b)->j);
}
static inline int dGbucket(float dG,int max) {
  /* Added in v1.32.  Previously had min2(-min2(0,2*dG),max)
     with min2 declared as int, but it segfaulted dGsCounts64
     in the case of dG calculation going to infinity
     (e.g. user had set all parameters to 0, causing a
     log of 0 to be attempted in deltaG_table) and
     the cast being undefined: could get 0x80000000 which doesn't
     play nicely with sign flipping etc */
  if (dG >= 0) return 0;
  dG *= -2.0;
  if (dG >= max) return max;
  /* OK, safe to cast */
  int dGi = dG;
  return dGi > max ? max : dGi; /* repeat just in case */
}
#include "bit-choice.h"
int checkLenLimit(int maxLen) { /* return 0 if OK */
  if(maxLen>PrimerLimit) {
    fprintf(stderr,">%d-base primers not yet supported",PrimerLimit);
#ifdef CPU_64bit
#if !Has_128bit
    fprintf(stderr," by this compiler.\n(128 bases can be supported if you compile the program with GCC or CLANG)");
#endif
#else
    fprintf(stderr," in the 32-bit version.\n(The 64-bit version supports up to 128 bases.)");
#endif
    fprintf(stderr,"\n"); return -1;
  } return 0;
}

void printFASTA(AllPrimers ap,FILE *f,const int *pools,int poolNo) {
  int i; for(i=0; i<ap.np; i++) if(pools[i]==poolNo) {
      fprintf(f,">%s\n",(ap.names[i]?ap.names[i]:"(no name)"));
      printBasesMaybeD(ap,i,f); fputc('\n',f);
    }
}

static void reportUnrecognisedBase(unsigned char b) {
  static char ignoredBasesReported[256]={0}; // TODO: when running interactively, if the user wants "another go" with another set of primers, consider resetting this array
  if(!ignoredBasesReported[b] && b>' ') {
    ignoredBasesReported[b] = 1;
    fprintf(stderr,"Ignoring unrecognised base %c\n",b);
  }
}
