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
/* Version of 64-128.h for x64 machines w/out 128-bit type
   (TODO: emulate the 128-bit type?)

   These files should be included ONLY by bit-common.h.
   The general interface is in all-primers.h.
*/

enum { PrimerLimit = 64 };
typedef struct {
  MaybeDegeneratePrimer64* forward;
  MaybeDegeneratePrimer64* backward;
  MaybeDegeneratePrimer64* tags; int *whichTag;
  char* *names; void *rawData; int np; int maxLen;
} AllPrimers;
void freeAllPrimers(AllPrimers ap) {
  if(ap.np >= 0) {
    free(ap.names); free(ap.forward);
    free(ap.backward); free(ap.tags);
    free(ap.whichTag); free(ap.rawData);
  }
}
static inline int sizeofMDPrimer(int maxLen) {
  return sizeof(MaybeDegeneratePrimer64);
}
AllPrimers loadFASTA(FILE *f) {
  AllPrimers r; /* sets r.np=-1 on failure */
  char *loadAndClose(FILE *f);
  int numPrimers(const char*,int*,int*);
  r.rawData = loadAndClose(f);
  if(!r.rawData) {
    fputs("Could not load file\n",stderr); r.np = -1;
    return r;
  }
  int numTags=0;
  r.np = numPrimers(r.rawData, &r.maxLen, &numTags);
  if(r.np<0) return r; /* err already printed */
  r.forward = malloc(r.np * sizeofMDPrimer(r.maxLen));
  r.backward = malloc(r.np*sizeofMDPrimer(r.maxLen));
  r.whichTag = malloc(r.np*sizeof(int));
  r.tags = calloc(numTags,sizeofMDPrimer(r.maxLen));
  r.names = malloc(r.np*sizeof(char*));
  if(memFail(r.forward,r.backward,r.tags,r.names,r.rawData,_memFail)) { r.np=-1; return r; }
  parseFASTA64(r.rawData,r.forward,r.tags,r.names);
  int i; for(i=0; i<r.np; i++) r.backward[i] = MaybeDegeneratePrimerReverse64(r.forward[i]);
  return r;
}
void addTags(AllPrimers ap) {
  int i;
  for(i=0; i<ap.np; i++) if(ap.whichTag[i]>=0) {
    MaybeDegeneratePrimer64 tag=ap.tags.p64[ap.whichTag[i]];
    MaybeDegeneratePrimerTag64(ap.forward.p64+i,tag);
    MaybeDegeneratePrimerTag64B(ap.backward.p64+i,tag);
  }
}
void removeTags(AllPrimers ap) {
  int i;
  for(i=0; i<ap.np; i++) if(ap.whichTag[i]>=0) {
    MaybeDegeneratePrimer64 tag=ap.tags.p64[ap.whichTag[i]];
    MaybeDegeneratePrimerRmTag64(ap.forward.p64+i,tag);
    MaybeDegeneratePrimerRmTag64B(ap.backward.p64+i,tag);
  }
}
void printCounts(AllPrimers ap) {
  counts64(ap.forward,ap.backward,ap.np);
}
void printPooledCounts(AllPrimers ap,const int *pools,const int *precalcScores) { /* precalcScores==NULL ok */
  pCounts64(ap.forward,ap.backward,ap.np,pools,precalcScores);
}
void printBonds(AllPrimers ap,FILE *f,int threshold,const int *pools) {
  printBonds64(ap.forward,ap.backward,ap.np,f,threshold,ap.names,pools);
  if (f!=stdout) fclose(f);
}
int* triangle(AllPrimers ap) {
  return triangle64(ap.forward,ap.backward,ap.np);
}

void printFASTA(AllPrimers ap,FILE *f,const int *pools,const int poolNo);
static void printBasesMaybeD(AllPrimers ap,int n,FILE *f) {
  printBases64MaybeD(ap.forward[n],f);
}

int NumPossibilities_32bases(AllPrimers ap,int n) {
  return NumPossibilities64MaybeD_32bases(ap.forward[n]);
}
int Make2bit(AllPrimers ap,int n,int useBackward,int doComplement,ULL *out,ULL *outValid,int possNo,int nPoss) {
  MaybeDegeneratePrimer64 p = useBackward ? ap.backward[n] : ap.forward[n];
  if(doComplement) PrimerComplement64MaybeD(&p);
  return Make2bitFrom64D(upgradeToDegenerate64(p),out,outValid,possNo,nPoss);
}

void dGprintBonds(AllPrimers ap,FILE *f,float threshold,const int *pools,const float *table) {
  dGprintBonds64(ap.forward,ap.backward,ap.np,f,threshold,ap.names,pools,table);
  if (f!=stdout) fclose(f);
}
int* dGtriangle(AllPrimers ap,const float *table) {
  return dGtriangle64(ap.forward,ap.backward,ap.np,table);
}
int dGprintPooledCounts(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f) {
  return dGpCounts64(ap.np,pools,precalcScores,f);
}
void dGandScoreCounts(AllPrimers ap,const float *table,FILE *f) {
  return dGsCounts64(ap.forward.p64,ap.backward.p64,ap.np,table,f);
}

void printStats(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f) {
  pStats64(ap.forward.p64,ap.backward.p64,ap.np,pools,precalcScores,f);
}
void dGprintStats(AllPrimers ap,const int *pools,const int *precalcScores,FILE *f) {
  pStats64dG(ap.np,pools,precalcScores,f);
}
