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
/* This is a substitute .h for modules that
   don't include bit-choice.h (which must be included only
   by bit-common.c) */
typedef struct {
  void *forward,*backward,*tags; /* not void* but will do */
  int *whichTag; char* *names; void *rawData;
  int np; int maxLen;
} AllPrimers;

AllPrimers loadFASTA(FILE *f);
void addTags(AllPrimers ap);
void removeTags(AllPrimers ap);
void printCounts(AllPrimers ap,FILE *f);
int printPooledCounts(AllPrimers ap,const int *pools,const int *precalcScores); /* precalcScores==NULL ok */
void printBonds(AllPrimers ap,FILE *f,int threshold,const int *pools); /* pools=NULL ok (does whole lot), otherwise limits to pairs that are in the same pool */
void printFASTA(AllPrimers ap,FILE *f,const int *pools,const int poolNo);
void freeAllPrimers(AllPrimers ap);

typedef struct { /* PS_cache: things that don't vary with number of pools */
  int *scores; int *primerMove_depends_on;
  int *fix_to_pool; int fix_min_pools;
} PS_cache;
int* split_into_pools(AllPrimers ap,int numPools,int timeLimit,PS_cache cache,int seedless,const float *table,int maxCount);
int suggest_num_pools(AllPrimers ap,PS_cache cache,const float *table);
PS_cache PS_precalc(AllPrimers ap,const float *table,const char *overlappingAmplicons,const int *primerNoToAmpliconNo,int nAmplicons); /* nAmplicons=0 ok if discounting that; table==NULL ok if not doing dG */
void PS_free(PS_cache c);

int* triangle(AllPrimers ap); // used by split_into_pools
char* GetOverlappingAmplicons(AllPrimers ap,FILE*genome,int* *primerNoToAmpliconNo,int *nAmp,int maxAmpliconLen,FILE* allAmps,int allAmpsIsMultiPLX,int ignoreVars); // so overlap, primerNoToAmpliconNo and nAmp can be passed to split_into_pools

void printBasesMaybeD(AllPrimers ap,int n,FILE *f);
int NumPossibilities_32bases(AllPrimers ap,int n); // used in amplicon.c
#include "bit-basics.h"
int Make2bit(AllPrimers ap,int n,int useBackward,int doComplement,ULL *out,ULL *outValid,int possNo,int nPoss); // ditto
void printBasesMaybeD(AllPrimers ap,int n,FILE *f);
void dGprintBonds(AllPrimers ap,FILE *f,float threshold,const int *pools,const float *table);
int* dGtriangle(AllPrimers ap,const float *table);
int dGprintPooledCounts(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f);
void dGandScoreCounts(AllPrimers ap,const float *table,FILE *f);
void printStats(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f);
void dGprintStats(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f);
