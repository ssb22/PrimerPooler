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
/* thirty-two to sixty-four dot h is automatically generated
   from sixty-four to one-hundred-and-twenty-eight dot h,
   with these two numbers halved throughout.

   These files should be included ONLY by bit-common.h.
   The general interface is in all-primers.h.
*/

enum { PrimerLimit = 128 }; /* max length of a primer */
typedef struct {
  union { /* array of primers read "forward" (5'-3') */
    MaybeDegeneratePrimer64* p64;
    MaybeDegeneratePrimer128* p128;
  } forward;
  union { /* same primers read "backward" (3'-5')
             (not to be confused with "reverse primer") */
    MaybeDegeneratePrimer64* p64;
    MaybeDegeneratePrimer128* p128;
  } backward;
  union { /* array of tags (read 5'-3') */
    MaybeDegeneratePrimer64* p64;
    MaybeDegeneratePrimer128* p128;
  } tags;
  int *whichTag; /* which tag no. applies to primer N (-1 for none) */
  char* *names; /* pointers to primer names */
  void *rawData; /* for above pointers to point into */
  int np;     /* total number of primers */
  int maxLen; /* length of longest primer */
} AllPrimers;
void freeAllPrimers(AllPrimers ap) {
  if(ap.np >= 0) {
    free(ap.names); free(ap.forward.p64);
    free(ap.backward.p64); free(ap.tags.p64);
    free(ap.whichTag); free(ap.rawData);
  }
}
static inline int sizeofMDPrimer(int maxLen) {
  return (maxLen <= 64) ? sizeof(MaybeDegeneratePrimer64):sizeof(MaybeDegeneratePrimer128);
}
AllPrimers loadFASTA(FILE *f) {
  AllPrimers r; /* sets r.np=-1 on failure */
  char *loadAndClose(FILE *f);
  int numPrimers(const char*,int*,int*); /*load-common.c*/
  r.rawData = loadAndClose(f);
  if(!r.rawData) {
    fputs("Could not load file\n",stderr); r.np = -1;
    return r;
  }
  int numTags=0;
  r.np = numPrimers(r.rawData, &r.maxLen, &numTags);
  if(r.np<0) return r; /* err already printed */
  r.forward.p64 = malloc(r.np * sizeofMDPrimer(r.maxLen));
  r.backward.p64 = malloc(r.np*sizeofMDPrimer(r.maxLen));
  r.whichTag = malloc(r.np*sizeof(int));
  r.tags.p64 = calloc(numTags,sizeofMDPrimer(r.maxLen));
  r.names = malloc(r.np*sizeof(char*));
  if(memFail(r.forward.p64,r.backward.p64,r.tags.p64,r.names,r.rawData,_memFail)) { r.np=-1; return r; }
  if(r.maxLen <= 64) {
    parseFASTA64(r.rawData,r.forward.p64,r.tags.p64,r.whichTag,r.names);
    int i; for(i=0; i<r.np; i++) r.backward.p64[i] = MaybeDegeneratePrimerReverse64(r.forward.p64[i]);
  }
  else {
    parseFASTA128(r.rawData,r.forward.p128,r.tags.p128,r.whichTag,r.names);
    int i; for(i=0; i<r.np; i++) r.backward.p128[i] = MaybeDegeneratePrimerReverse128(r.forward.p128[i]);
  }
  return r;
}
void addTags(AllPrimers ap) {
  int i;
  if(ap.maxLen <= 64) {
    for(i=0; i<ap.np; i++) if(ap.whichTag[i]>=0) {
      MaybeDegeneratePrimer64 tag=ap.tags.p64[ap.whichTag[i]];
      MaybeDegeneratePrimerTag64(ap.forward.p64+i,tag);
      MaybeDegeneratePrimerTag64B(ap.backward.p64+i,tag);
    }
  } else
    for(i=0; i<ap.np; i++) if(ap.whichTag[i]>=0) {
      MaybeDegeneratePrimer128 tag=ap.tags.p128[ap.whichTag[i]];
      MaybeDegeneratePrimerTag128(ap.forward.p128+i,tag);
      MaybeDegeneratePrimerTag128B(ap.backward.p128+i,tag);
    }
}
void removeTags(AllPrimers ap) {
  int i;
  if(ap.maxLen <= 64) {
    for(i=0; i<ap.np; i++) if(ap.whichTag[i]>=0) {
      MaybeDegeneratePrimer64 tag=ap.tags.p64[ap.whichTag[i]];
      MaybeDegeneratePrimerRmTag64(ap.forward.p64+i,tag);
      MaybeDegeneratePrimerRmTag64B(ap.backward.p64+i,tag);
    }
  } else
    for(i=0; i<ap.np; i++) if(ap.whichTag[i]>=0) {
      MaybeDegeneratePrimer128 tag=ap.tags.p128[ap.whichTag[i]];
      MaybeDegeneratePrimerRmTag128(ap.forward.p128+i,tag);
      MaybeDegeneratePrimerRmTag128B(ap.backward.p128+i,tag);
    }
}
void printCounts(AllPrimers ap,FILE *f) {
  if(ap.maxLen <= 64)
    counts64(ap.forward.p64,ap.backward.p64,ap.np,f);
  else counts128(ap.forward.p128,ap.backward.p128,ap.np,f);
}
int printPooledCounts(AllPrimers ap,const int *pools,const int *precalcScores) { /* precalcScores==NULL ok */
  if(ap.maxLen <= 64)
    return pCounts64(ap.forward.p64,ap.backward.p64,ap.np,pools,precalcScores);
  else return pCounts128(ap.forward.p128,ap.backward.p128,ap.np,pools,precalcScores);
}
void printBonds(AllPrimers ap,FILE *f,int threshold,const int *pools) {
  if(ap.maxLen <= 64)
    printBonds64(ap.forward.p64,ap.backward.p64,ap.np,f,threshold,ap.names,pools);
  else printBonds128(ap.forward.p128,ap.backward.p128,ap.np,f,threshold,ap.names,pools);
  if (f!=stdout) fclose(f);
}
int* triangle(AllPrimers ap) {
  if(ap.maxLen <= 64)
    return triangle64(ap.forward.p64,ap.backward.p64,ap.np);
  else return triangle128(ap.forward.p128,ap.backward.p128,ap.np);
}

void printFASTA(AllPrimers ap,FILE *f,const int *pools,const int poolNo);
void printBasesMaybeD(AllPrimers ap,int n,FILE *f) {
  if(ap.maxLen <= 64)
    printBases64MaybeD(ap.forward.p64[n],f);
  else printBases128MaybeD(ap.forward.p128[n],f);
}

int NumPossibilities_32bases(AllPrimers ap,int n) {
  /* forward or backward should yield same result */
  if(ap.maxLen <= 64) return NumPossibilities64MaybeD_32bases(ap.forward.p64[n]);
  else return NumPossibilities128MaybeD_32bases(ap.forward.p128[n]);
}
int Make2bit(AllPrimers ap,int n,int useBackward,int doComplement,ULL *out,ULL *outValid,int possNo,int nPoss) {
  if(ap.maxLen <= 64) {
    MaybeDegeneratePrimer64 p = useBackward ? ap.backward.p64[n] : ap.forward.p64[n];
    if(doComplement) PrimerComplement64MaybeD(&p);
    return Make2bitFrom64D(upgradeToDegenerate64(p),out,outValid,possNo,nPoss);
  } else {
    MaybeDegeneratePrimer128 p = useBackward ? ap.backward.p128[n] : ap.forward.p128[n];
    if(doComplement) PrimerComplement128MaybeD(&p);
    return Make2bitFrom128D(upgradeToDegenerate128(p),out,outValid,possNo,nPoss);
  }
}

void dGprintBonds(AllPrimers ap,FILE *f,float threshold,const int *pools,const float *table) {
  if(ap.maxLen <= 64)
    dGprintBonds64(ap.forward.p64,ap.backward.p64,ap.np,f,threshold,ap.names,pools,table);
  else dGprintBonds128(ap.forward.p128,ap.backward.p128,ap.np,f,threshold,ap.names,pools,table);
  if (f!=stdout) fclose(f);
}
int* dGtriangle(AllPrimers ap,const float *table) {
  if(ap.maxLen <= 64)
    return dGtriangle64(ap.forward.p64,ap.backward.p64,ap.np,table);
  else return dGtriangle128(ap.forward.p128,ap.backward.p128,ap.np,table);
}
int dGprintPooledCounts(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f) {
  if(ap.maxLen <= 64)
    return dGpCounts64(ap.np,pools,precalcScores,f);
  else return dGpCounts128(ap.np,pools,precalcScores,f);
}
void dGandScoreCounts(AllPrimers ap,const float *table,FILE *f) {
  if(ap.maxLen <= 64)
    return dGsCounts64(ap.forward.p64,ap.backward.p64,ap.np,table,f);
  else return dGsCounts128(ap.forward.p128,ap.backward.p128,ap.np,table,f);
}

void printStats(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f) { /* precalcScores==NULL ok */
  if(ap.maxLen <= 64)
    pStats64(ap.forward.p64,ap.backward.p64,ap.np,pools,precalcScores,f);
  else pStats128(ap.forward.p128,ap.backward.p128,ap.np,pools,precalcScores,f);
}
void dGprintStats(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f) { /* precalcScores==NULL NOT ok in current implementation */
  if(ap.maxLen <= 64)
    pStats64dG(ap.np,pools,precalcScores,f);
  else pStats128dG(ap.np,pools,precalcScores,f);
}
