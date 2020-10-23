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
#ifdef __APPLE__
#define _FORTIFY_SOURCE 0 /* Mac OS 10.7 OpenMP bug workaround */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include "ansi.h"
#include "bit-basics.h"
#include "debug.h"
#include "openmp.h"
#include "memcheck.h"
#include "numbers.h"

#define PARALLELIZE_CHROMOSOMES 1

typedef uint32_t b32;
static inline b32 byteSwap32(b32 i) {
  switch(0){case 0:case sizeof(b32)==4:;} /* must be EXACTLY 4 bytes for this code to work */
  i = (i>>16) | (i<<16);
  return ((i&0xFF00FF00)>>8) | ((i&0x00FF00FF)<<8);
}
static b32 read2bSwap(FILE *f,int byteSwap) {
  b32 p; if(!fread(&p,sizeof(p),1,f)) {
    fputs("Read error\n",stderr); return 0;
  }
  if(byteSwap) p=byteSwap32(p);
  return p;
}
static int read_2bit_nSeqs(FILE *f,int *byteSwap,long *seqPtr) {
  switch(0){case 0:case sizeof(b32)==4:;} /* as above */
  switch(read2bSwap(f,0)) {
  case 0x1A412743: *byteSwap=0; break;
  case 0x4327411A: *byteSwap=1; break;
  default: fputs("Invalid signature (is this really a .2bit genome file?)\n",stderr); return 0;
  }
  if(read2bSwap(f,0 /* *byteSwap, but doesn't matter if we're comparing result against 0 */)) { fputs("Invalid version\n",stderr); return 0; }
  int seqCount = read2bSwap(f,*byteSwap);
  read2bSwap(f,0); // reserved (again *byteSwap not nec)
  *seqPtr = ftell(f); return seqCount;
}
typedef char SeqName[256];
SeqName lastSequenceNameRead={0};
int variants_skipped;
static int is_variant() {
  if (strchr(lastSequenceNameRead,'_') != NULL
      || strchr(lastSequenceNameRead,'-') != NULL) {
    ++variants_skipped;
    return 1;
  } else return 0;
}
static int read_2bit_nBases(FILE *f,int byteSwap,long *seqPtr,b32* *unknownStart,b32* *unknownLen,b32 *nBases,int *isVariant,int ignoreVars) {
  if (fseek(f,*seqPtr,SEEK_SET)) { fputs("Seek error reading sequence offset\n",stderr); return 0; }
  int seqNameLen = getc(f);
  if(!fread(lastSequenceNameRead,1,seqNameLen,f)) { fputs("Error reading sequence name\n",stderr); return 0; }
  lastSequenceNameRead[seqNameLen]=0;
  if(ignoreVars) *isVariant = is_variant();
  else *isVariant = 0;
#ifdef Debug_ChromosomeCheck
  *isVariant = strcmp(lastSequenceNameRead,Debug_ChromosomeCheck); /* treat any OTHER chromosome as a variant we won't read, for debugging with just one */
#endif
  long offset=read2bSwap(f,byteSwap);
  if(!offset) { fputs("No sequence offset\n",stderr); return 0; }
  *seqPtr = ftell(f);
  if(fseek(f,offset,SEEK_SET)) { fputs("Seek error loading sequence\n",stderr); return 0; }
  *nBases=read2bSwap(f,byteSwap);
  #ifdef Debug_ChromosomeCheck
  if(!*isVariant) fprintf(stderr,"Chromosome size is %d\n",*nBases);
  #endif
  b32 nUnknown = read2bSwap(f,byteSwap);
  *unknownStart = malloc(sizeof(b32)*(nUnknown+1)); /* we'll add an extra one at the end so don't have to keep checking unknownPtr<nUnknown */
  *unknownLen = malloc(sizeof(b32)*(nUnknown+1));
  if(memFail(*unknownLen,*unknownStart,_memFail)) return 0;
  b32 i; for(i=0; i<nUnknown; i++) (*unknownStart)[i]=read2bSwap(f,byteSwap); (*unknownStart)[i]=*nBases;
  for(i=0; i<nUnknown; i++) { (*unknownLen)[i] = read2bSwap(f,byteSwap); assert((*unknownLen)[i]); } (*unknownLen)[i]=0; /* if get unknownLen==0, need to adjust the 'else' branch of 'baseNo < *unknownStart' below to allow for this possibility (or just remove that block, running mmove on unknownStart to keep it in sync) */
  if(fseek(f,read2bSwap(f,byteSwap)*4*2 + 4,SEEK_CUR)) // skip 'masked blocks' (TODO: are these ever relevant?) and the 4-byte 'reserved' word
    { fputs("Seek error skipping mask\n",stderr); return 0; }
  return 1; // and ftell = start; *seqPtr = next seq, unknown.. needs free
}

static inline void addBase(bit64 *buf,bit64 *valid,unsigned char byte,int *basesLeftInByte) {
  /* This function has to be FAST: seriously inner-loop.
     For ease of binary search in amplicons.c, bases are
     shifted into buf from LEFT (not right as in 64.h),
     so buf contains the last few bases IN REVERSE from
     the genome cursor (which is an 'end' cursor).  */
  *buf = ((*buf) >> 2) | ((((bit64)byte >> (2*--*basesLeftInByte)) & (bit64)3) << 62);
  if(*valid != ~(bit64)0) /* (only at start, so put the 'if' around it to save a couple of instructions) */
    *valid = ((*valid) >> 2) | ((bit64)3 << 62);
}

static inline void addFastaBase(bit64 *buf,bit64 *valid,char letter) {
  int basesLeftInByte = 1, twobit;
  switch(letter) {
  case 'C': case 'c': twobit = 1; break;
  case 'A': case 'a': twobit = 2; break;
  case 'G': case 'g': twobit = 3; break;
  case ' ': case '\t': case '\r': case '\n': return;
  default: twobit = 0; /* TODO: might be degenerate bases like 'M'; use these in nonspecific-amplicon checks? (but hopefully they're using .2bit anyway) */
  }
  addBase(buf,valid,twobit,&basesLeftInByte);
}

void look(bit64,bit64,int,b32); /* could pass this into go_through_genome as a pointer, but doing so will slow us down; there's only one in the program so let's let -flto consider it for inlining */
int allocateSeqs(int nSeq); /* ditto */
int allocateAnotherSeq(int nSeq); /* ditto */

static int is_fasta(FILE *f) {
  // Check if we're looking at a FASTA file instead of a 2bit file.
  // Assume FASTA will begin with '>' or newline + '>' or BOM + '>'
  // If FASTA is detected, seek past the first '>'.  Otherwise rewind.
  char dat[5]; if(!fread(dat,4,1,f)) *dat=0;
  rewind(f); dat[4]=0;
  if (strspn(dat,"\r\n\xef\xbb\xbf>")) {
    while(fgetc(f)!='>') if(feof(f)) {
        fprintf(stderr,"FASTA file with no sequences??\n");
        rewind(f); return 0;
      }
    return 1;
  } else return 0;
}

static void readFastaSeqName(FILE *f) {
  // assume 'f' is positioned just after the '>'
  if(!fgets(lastSequenceNameRead,sizeof(SeqName),f))
    *lastSequenceNameRead = 0;
  if(lastSequenceNameRead[strlen(lastSequenceNameRead)-1]!='\n') {
    while (fgetc(f) != '\n') { if(feof(f)) break; }
  }
  lastSequenceNameRead[strcspn(lastSequenceNameRead," \t\r\n")]=0;
}
static void takeFastaSeqName(FILE *f,const char *buf) {
  strcpy(lastSequenceNameRead,buf+1); /* ignore '>' */
  lastSequenceNameRead[strcspn(lastSequenceNameRead," \t\r\n")]=0;
  if(buf[strlen(buf)-1]!='\n') {
    while (fgetc(f) != '\n') { if(feof(f)) break; }
  }
}

static SeqName* fasta_genome(FILE *f,int ignoreVars) {
  fprintf(stderr,"Reading genome from FASTA file\n(slower than .2bit; may take time)\n");
  int seqNo=0; char buf[80];
  SeqName *seqNames=NULL;
  while(!feof(f)) {
    if(seqNo) takeFastaSeqName(f,buf);
    else readFastaSeqName(f);
    if(ignoreVars && is_variant()) {
      while(fgets(buf,sizeof(buf),f) && *buf != '>');
      continue;
    }
    fprintf(stderr,"%s\n",lastSequenceNameRead);
    SeqName *lastSeqNames = seqNames;
    seqNames = realloc(seqNames,(seqNo+1)*sizeof(SeqName));
    if(!seqNames || allocateAnotherSeq(seqNo+1)==seqNo) {
      free(lastSeqNames);
      fprintf(stderr,"Genome metadata: Out of memory!\n"); break;
    }
    wrapped_memcpy(seqNames[seqNo],lastSequenceNameRead,sizeof(SeqName));
    bit64 curBuf=0,curValid=0; b32 baseNo=0;
    while(fgets(buf,sizeof(buf),f) && *buf != '>') {
      char *b;
      for(b=buf; *b; b++) {
        switch (*b) {
        case 'N': case 'n': curBuf = curValid = 0; break;
        default:
          addFastaBase(&curBuf,&curValid,*b);
          look(curBuf,curValid,seqNo,baseNo);
        } baseNo++;
      }
    }
    seqNo++;
  }
  fprintf(stderr,"End of FASTA genome scan\n"); return seqNames;
}

SeqName* go_through_genome(FILE *f,int ignoreVars) {
  variants_skipped = 0;
  if(is_fasta(f)) return fasta_genome(f,ignoreVars);
  int byteSwap=0; long seqPtr=0; // =0 to suppress warning
  int nSeq=read_2bit_nSeqs(f,&byteSwap,&seqPtr);
  if(!allocateSeqs(nSeq)) return NULL;
  int seqsDone = 0;
  SeqName *seqNames=NULL; int numSeqNames=0;
  time_t start = time(NULL);
  time_t nextDisplay = start + 1;
  if(omp_get_max_threads() > 1) {
    fprintf(stderr,"Parallelising scan (up to %d chromosomes simultaneously)\n",omp_get_max_threads());
  }
  int seqNo; char progressBuf[80]={0}; /* TODO: different screen widths?  (low priority because it could just scroll, but 8 cores on a narrow terminal could be messy) */ enum { ProgWidthPerThread = 13 /* sequence name width is this minus 5; try to divide into the screen width and also allow for the possiblity of narrower terminals */ };
  #if PARALLELIZE_CHROMOSOMES && defined(_OPENMP)
  #pragma omp parallel for schedule(dynamic)
  #endif
  for(seqNo=0;seqNo<nSeq;seqNo++) {
    time_t nextThreadUpdate = time(NULL)+1; int tNum = omp_get_thread_num(); char *pgBuf = ((tNum+1)*ProgWidthPerThread > sizeof(progressBuf)) ? NULL : (progressBuf+tNum*ProgWidthPerThread); int isRHS=(tNum==omp_get_num_threads()-1) || ((tNum+2)*ProgWidthPerThread)>sizeof(progressBuf);
    b32 *allUnknownStart=0,*allUnknownLen=0,nBases=0; /* =0 for old compilers (don't warn) */
    int isVariant,renumberedSeqNo=0; /* =0 for old compilers (don't warn) */
    #if PARALLELIZE_CHROMOSOMES && defined(_OPENMP)
    long ft=0; /* =0 for old compilers (don't warn) */
    #endif
    b32 baseNo = 0, *unknownStart=0, *unknownLen=0; /* last two =0 for old compilers */
    int bufBytes=0;/* =0 for old compilers (don't warn) */
    char *buf=NULL;
    if(numSeqNames && !seqNames) continue; /* would have been 'break' below w/out OpenMP */
    #if PARALLELIZE_CHROMOSOMES && defined(_OPENMP)
    #pragma omp critical
    #endif
    {
    if(read_2bit_nBases(f,byteSwap,&seqPtr,&allUnknownStart,&allUnknownLen,&nBases,&isVariant,ignoreVars)) {
    if(isVariant) { free(allUnknownStart); free(allUnknownLen); } else {
    if((renumberedSeqNo = seqsDone++) >= numSeqNames) {
      numSeqNames = (numSeqNames+1)<<1;
      seqNames = realloc(seqNames,numSeqNames*sizeof(SeqName)); memFail(seqNames,allUnknownLen,allUnknownStart,_memFail);
    } if(seqNames) wrapped_memcpy(seqNames[renumberedSeqNo],lastSequenceNameRead,sizeof(SeqName));
    unknownStart=allUnknownStart;unknownLen=allUnknownLen;
    bufBytes = nBases/4;if(bufBytes) buf=malloc(bufBytes);
    if(buf) {
      if(!fread(buf,1,bufBytes,f)) {
        fprintf(stderr,"\nError reading %d bytes (current pointer is &%lx)\nCorrupt genome file?\n",bufBytes,ftell(f));
        free(buf); if(seqNames) free(seqNames); seqNames=NULL; /* would be 'return NULL' w/out OpenMP */
      }
    } else bufBytes=0;
    #if PARALLELIZE_CHROMOSOMES && defined(_OPENMP)
    if(f) ft = ftell(f);
    #endif
    } } else isVariant=1; }
    if(isVariant || !seqNames) continue;
    int bufPtr = 0;
    bit64 curBuf=0,curValid=0; int basesLeftInByte = 0;
    unsigned char byte = 0; // =0 to suppress warning
    while(baseNo<nBases) {
      /* OK, let's go through all the bases.  And try not
         to think of the daft 2001-ish "Internet meme".
         TODO: it might be possible to manually unroll
         this loop a bit so fewer tests are needed on
         baseNo etc. */
      if(!basesLeftInByte) {
        if(bufPtr==bufBytes)
          #if PARALLELIZE_CHROMOSOMES && defined(_OPENMP)
          #pragma omp critical
          #endif
          {
          #if PARALLELIZE_CHROMOSOMES && defined(_OPENMP)
            fseek(f,ft++,SEEK_SET);
          #endif
            byte = getc(f);
          } else byte=buf[bufPtr++];
        basesLeftInByte = 4;
        if(pgBuf && time(NULL) >= nextThreadUpdate)
          #if PARALLELIZE_CHROMOSOMES && defined(_OPENMP)
          #pragma omp critical
          #endif
          {
          int outputted=sprintf(pgBuf,"%*s %2d%%",ProgWidthPerThread-5,seqNames[renumberedSeqNo],(int)((float)baseNo*100.0/(float)nBases));
          if(!isRHS) pgBuf[outputted]=' ';
          nextThreadUpdate = time(NULL)+1;
        }
        if(time(NULL) >= nextDisplay)
          #if PARALLELIZE_CHROMOSOMES && defined(_OPENMP)
          #pragma omp critical
          #endif
          // (this comment added to work around an auto-indent bug in some Emacs versions)
          {
          if(time(NULL) >= nextDisplay) {
          if(omp_get_num_threads()==1) fputs("\rScanning ",stderr);
          else fputs("\r",stderr); /* Scanning message will have been printed above, and we have many columns to worry about */
          fputs(progressBuf,stderr); fflush(stderr);
          nextDisplay = time(NULL) + 2;
          }}
      }
      if(baseNo < *unknownStart) {
        addBase(&curBuf,&curValid,byte,&basesLeftInByte);
        look(curBuf,curValid,renumberedSeqNo,++baseNo);
      } else { /* we're in an 'unknown' region */
        --basesLeftInByte; /* ignore this one */
        if(++baseNo == *unknownStart + *unknownLen) {
          unknownStart++; unknownLen++;
          curBuf = curValid = 0;
        }
      }
    }
    if(buf) free(buf);
    free(allUnknownStart); free(allUnknownLen);
    if(pgBuf) memset(pgBuf,' ',ProgWidthPerThread-isRHS);
  }
  fprintf(stderr,"\rGenome scan complete");
  prnSeconds((long)(time(NULL)-start));
  fputs(clearEOL(),stderr);
  fputs("\n",stderr);
  return seqNames;
}

void output_genome_segment(FILE *f,int targetRenumberedSeqNo,b32 baseStart,int nBases,FILE *out,int ignoreVars) {
  /* Cut-down version of go_through_genome for use in reports */
  int byteSwap=0; long seqPtr=0;
  rewind(f);
  int nSeq=read_2bit_nSeqs(f,&byteSwap,&seqPtr);
  int seqsDone = 0, seqNo;
  for(seqNo=0;seqNo<nSeq;seqNo++) {
    int isVariant;
    b32 *allUnknownStart=0,*allUnknownLen=0, nb0;
    if(read_2bit_nBases(f,byteSwap,&seqPtr,&allUnknownStart,&allUnknownLen,&nb0,&isVariant,ignoreVars)) {
    free(allUnknownStart); free(allUnknownLen);
    if (!isVariant && (seqsDone++ == targetRenumberedSeqNo)) {
      fseek(f,--baseStart/4,SEEK_CUR); // 1st is 0 not 1
      int shift = 2*(baseStart & 3);
      int mask = (128|64) >> shift; shift = 6 - shift;
      do {
        unsigned char byte = getc(f);
        while(mask && nBases) {
          fputc("TCAG"[(byte >> shift) & 3], out);
          nBases--; shift-=2; mask>>=2;
        }
        mask = 128|64; shift = 6;
      } while(nBases);
      break;
    }}
  }
}
