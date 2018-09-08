/*
# This file is part of Primer Pooler v1.42 (c) 2016-18 Silas S. Brown.  For Wen.
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
/* This is a substitute .h for modules that
   don't include bit-choice.h (which must be included only
   by bit-common.c) */
typedef struct {
  void *forward,*backward,*tags; // not quite but will do
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

typedef struct { /* things that don't vary with number of pools */ int *scores; int *primerMove_depends_on; } PS_cache;
int* split_into_pools(AllPrimers ap,int numPools,int timeLimit,PS_cache cache,int seedless,const float *table,int maxCount);
int suggest_num_pools(AllPrimers ap,PS_cache cache,const float *table);
PS_cache PS_precalc(AllPrimers ap,const float *table,const char *overlappingAmplicons,const int *primerNoToAmpliconNo,int nAmplicons); /* nAmplicons=0 ok if discounting that; table==NULL ok if not doing dG */
void PS_free(PS_cache c);

int* triangle(AllPrimers ap); // used by split_into_pools
char* GetOverlappingAmplicons(AllPrimers ap,FILE*genome,int* *primerNoToAmpliconNo,int *nAmp,int maxAmpliconLen,FILE* allAmps,int allAmpsIsMultiPLX); // so overlap, primerNoToAmpliconNo and nAmp can be passed to split_into_pools

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
