/*
# This file is part of Primer Pooler v1.51 (c) 2016-19 Silas S. Brown.  For Wen.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include "all-primers.h"
#include "ansi.h"
#include "debug.h"
#include "genome.h"
#include "memcheck.h"
#include "numbers.h"

static int amplicons_from_primer_names(AllPrimers ap,int* *primerNoToAmpliconNo,int* *ampliconNoToFwd,int* *ampliconNoToRev) { /* returns nAmp; arrays need free if nAmp is non-0 */
  *ampliconNoToFwd=malloc(sizeof(int)*(ap.np/2));
  *ampliconNoToRev=malloc(sizeof(int)*(ap.np/2));
  *primerNoToAmpliconNo=malloc(sizeof(int)*ap.np);
  if(memFail(*ampliconNoToFwd,*ampliconNoToRev,*primerNoToAmpliconNo,_memFail)) return 0;
  memset(*primerNoToAmpliconNo,0xFF,sizeof(int)*ap.np);
  int nAmp=0, foundForward=0, foundBackward=0;
  int i,jStart=0; for(i=0; i<ap.np; i++) {
    const char *n = ap.names[i]; size_t l = strlen(n);
    if(l && (n[l-1]=='F' || n[l-1]=='f')) {
      foundForward = 1;
      if(i==jStart) jStart++; /* optimisation for if they're arranged as (f,r)(f,r)(f,r) in the input */
      int j; for(j=jStart; j<ap.np; j++) {
        if(j!=i && !strncmp(ap.names[j],n,l-1) && strchr("BbRr",ap.names[j][l-1]) && !ap.names[j][l]) {
          /* Do check for -B or -R, don't just take anything that's not -F, because
             they might have TaqMan probes as -P or something (new in v1.36; previous
             versions would be OK with this in the pooling stage but this overlap-check
             stage could get confused) */
          foundBackward = 1;
          #ifdef Debug_PrimerNamePrefix
          if(!strncmp(ap.names[i],Debug_PrimerNamePrefix,sizeof(Debug_PrimerNamePrefix)-1))
            fprintf(stderr,"Amplicon %d is P%d (%s) and %d (%s)\n",nAmp,i,ap.names[i],j,ap.names[j]);
          #endif
          (*ampliconNoToFwd)[nAmp] = i;
          (*ampliconNoToRev)[nAmp] = j;
          (*primerNoToAmpliconNo)[i] = (*primerNoToAmpliconNo)[j] = nAmp++;
          if(j==jStart && jStart++ == i+1) ++i; /* NOT i = new jStart, because the loop counter will also incr it */
          break;
        }
      }
    }
  } if(!nAmp) { free(*ampliconNoToFwd); free(*ampliconNoToRev); }
  SetColour(Bright,Cyan,Black); fprintf(stderr,"%d primer-sets found\n",nAmp); ResetColour();
  int reported_missing = 0;
  if(nAmp) {
    for(i=0; i<ap.np; i++) {
      if((*primerNoToAmpliconNo)[i] == -1) {
        if(!reported_missing) {
          fputs("The following primers were not matched into amplicon sets:\n(should you check the spelling and capitalisation?)\n",stderr);
          reported_missing = 1;
        }
        fprintf(stderr,"%s\n",ap.names[i]);
      }
    }
  } else if (!foundForward || !foundBackward) {
    /* 2016-09: a user in India thought you had to end
       primers with '-forward' and '-backward'.  Are my
       instructions really that unclear?  :-(
       (TODO: maybe we could parse the whole word after
       the final hyphen.  but would need to ensure other
       users understand if this happens.)  */
    fputs("Please end your forward primers with -F\nand your reverse primers with -R or -B as instructed\n",stderr);
  }
  return nAmp;
}

typedef struct {
  b32 baseEnd,baseStart; int ampNo, onOrOff;
  char *name; /* for reports */
} AmpEvent;
typedef struct {
  bit64 lhsBases; /* = p & minValid, for the bsearch (is actually the LAST len(minValid)/2 bases before the cursor, but we're reading backwards from the left see below) */
  bit64 p; bit64 valid; /* what we're looking for */
  int ampNo, onOrOff; /* what to do when we find it */
  int primerNo; char* name; /* for reports */
} PrimerToFind;
static int PF_cmp_func(const void *aP, const void *bP) {
  if(((PrimerToFind*)aP)->lhsBases < ((PrimerToFind*)bP)->lhsBases)
    return -1;
  else return (((PrimerToFind*)aP)->lhsBases > ((PrimerToFind*)bP)->lhsBases);
}
static int PF_srch_func(const void *keyP, const void *tableP) {
  bit64 k = *(bit64*)keyP,
    t = ((PrimerToFind*)tableP)->lhsBases;
  if(k < t) return -1;
  else return k > t;
}
static int* didFind;
static int PF_by_didFind(const void *aP, const void *bP) {
  /* this is for the checkPF report at the end */
  int r = didFind[((PrimerToFind*)aP)->primerNo] - didFind[((PrimerToFind*)bP)->primerNo]; /* <0 if A comes first */
  if(!r) r = ((PrimerToFind*)aP)->primerNo - ((PrimerToFind*)bP)->primerNo;
  return r;
}

/* These need to be at module level so that look() can get
   at them quickly w/out having to pass too much stuff
   around the inner loop (unless indirect off a struct) */
static PrimerToFind *primerVariantsToFind;
static int nPrimerVariantsToFind; static bit64 minValid;
typedef struct {
  AmpEvent *events; size_t ptr,size;
} EventList;
static EventList *eventLists; size_t numEventLists;

#if defined(Debug_BaseCheck) || defined(Debug_AmpliconNo)
static void debugPrnRTL(bit64 bases,bit64 valid) {
  bit64 mask = 3;
  while(mask) {
    if(mask & valid)
      fputc("TCAG"[(bases & mask) >> (62-leading0_64(mask))],stderr);
    mask <<= 2;
  }
}
#endif

int allocateSeqs(int nSeq) {
  eventLists = calloc(nSeq,sizeof(EventList));
  if(!eventLists) nSeq = 0;
  numEventLists = nSeq;
  return nSeq;
}
int allocateAnotherSeq(int nSeq) {
  /* Called by the FASTA alternative genome reader, which
     doesn't know in advance how many sequences it'll get */
  if (nSeq == 1) return allocateSeqs(nSeq); /* first call */
  EventList *el2 = realloc(eventLists, nSeq*sizeof(EventList));
  if (el2) {
    eventLists = el2;
    memset(&(eventLists[nSeq-1]),0,sizeof(EventList));
    numEventLists = nSeq;
    return nSeq;
  } else return numEventLists;
}

void look(bit64 buf,bit64 valid,int seqNo,b32 baseEnd) {
  /* Called for every position in the genome:
     find which (if any) of the primers we're looking for
     ends at this position, and deal with it.
     For ease of binary search, bases are shifted into
     buf from the LEFT (not from the right as in 64.h).
     We'll do a binary search on our SHORTEST primer len
     and linear search the rest (saves having to launch a
     separate binary search for each length: it's rare to
     have a collection of primers where many share the
     same tail that's as long as the shortest primer in
     the set, so the linear search should be quite short)
  */
#ifdef Debug_BaseCheck
  if(baseEnd==Debug_BaseCheck) { debugPrnRTL(buf,valid); fprintf(stderr,"\n"); }
#endif
  bit64 lhsBases = buf & minValid;
  PrimerToFind *found = bsearch(&lhsBases,primerVariantsToFind,nPrimerVariantsToFind,sizeof(*primerVariantsToFind),PF_srch_func);
  if (!found) return; /* (as soon as possible) */
  while(found>primerVariantsToFind && lhsBases==found[-1].lhsBases) found--; /* (bsearch might not land at the START of a section with equal lhs bases) */
  for(; found<primerVariantsToFind+nPrimerVariantsToFind && found->lhsBases == lhsBases; found++) {
    if((buf & found->valid) == found->p && (found->valid & valid) == found->valid) {
      /* OK, all up-to-32 bases match */
      didFind[found->primerNo]++;
      EventList *l = eventLists + seqNo;
      assert(seqNo < numEventLists);
      if(l->ptr == l->size) {
        l->size = (l->size | 1) << 1;
        AmpEvent *events2 = (l->size > l->ptr) ? realloc(l->events,l->size*sizeof(AmpEvent)) : NULL; /* NULL if integer overflow (hopefully unlikely!) */
        if(memFail(events2,_memFail)) {
          fputs("Won't be able to record any more amplicon events\n",stderr); // TODO: and stop scanning the genome (but without making the error-handling code slow it down; use longjmp??)
          l->size = l->ptr;
        } else l->events = events2;
      } if(l->ptr < l->size) {
        #ifdef Debug_AmpliconNo
        extern SeqName lastSequenceNameRead;
        if(Debug_AmpliconNo(found->ampNo))
          fprintf(stderr,"Event #%lu: amplicon %d state %d baseEnd=%s:%u \n",l->ptr,found->ampNo,found->onOrOff,lastSequenceNameRead,baseEnd);
        #endif
        l->events[l->ptr].baseEnd = baseEnd;
        l->events[l->ptr].baseStart = baseEnd - (64-__builtin_ctzll(found->valid))/2 + 1; /* +1 to bring into line with what UCSC browser does */
        l->events[l->ptr].ampNo = found->ampNo;
        l->events[l->ptr].onOrOff = found->onOrOff;
        l->events[l->ptr++].name = found->name;
      }}}}

static PrimerToFind* populatePF(AllPrimers ap,int *ampliconNoToFwd,int *ampliconNoToRev,int nAmp,int *nRet) {
  /* For how many primers do we want to search through the genome?
     (I'm assuming degenerate bases are rare so we can write out all combinations here;
     TODO: if this runs out of memory, try handling the degenerate primers with brute-force matching instead) */
  int ampNo,nToFind=0; for(ampNo=0; ampNo<nAmp; ampNo++) nToFind += NumPossibilities_32bases(ap,ampliconNoToFwd[ampNo]) + NumPossibilities_32bases(ap,ampliconNoToRev[ampNo]);
  nToFind *= 2; // because we want to check the negative strand as well
  /* and write them all out into the struct array: */
  PrimerToFind *pF=malloc(nToFind*sizeof(PrimerToFind));
  if(memFail(pF,_memFail)) return 0;
  int outP = 0,doneTruncate = 0;
  const char truncateMsg[]="The genome-overlap search function currently supports max 32 bases,\nso it'll look at only the last 32 bases of longer primers:\n";
  for(ampNo=0; ampNo<nAmp; ampNo++) {
    int isFwd; for(isFwd=0; isFwd<=1; isFwd++) {
      int primerNo=(isFwd?ampliconNoToFwd:ampliconNoToRev)[ampNo];
#ifdef Debug_AmpliconNo
      if(Debug_AmpliconNo(ampNo)) {
        fprintf(stderr,"p#=% 8d ",primerNo); printBasesMaybeD(ap,primerNo,stderr); fputs(":\n",stderr);
      }
#endif
      int nPo=NumPossibilities_32bases(ap,primerNo);
      int i,didTruncate=0,reverseAndComplement;
      for(i=0; i<nPo; i++) {
        for(reverseAndComplement=0; reverseAndComplement<=1; reverseAndComplement++) {
          didTruncate = Make2bit(ap,primerNo,reverseAndComplement,reverseAndComplement,&(pF[outP].p),&(pF[outP].valid),i,nPo);
#ifdef Debug_AmpliconNo
          if(Debug_AmpliconNo(ampNo)) {
            fprintf(stderr,"%d/%d (rc=%d): ",i,nPo,reverseAndComplement); debugPrnRTL(pF[outP].p,pF[outP].valid); fprintf(stderr," -> %d\n",outP); /* offset in primerVariantsToFind */
          }
#endif
          pF[outP].ampNo = ampNo; pF[outP].onOrOff = ((isFwd==reverseAndComplement)?-1:1)*(isFwd?1:-1) * (1+(isFwd == reverseAndComplement)); // this should get +1,-1 for +ve strand 1st, or +2,-2 for -ve strand 1st
          pF[outP].primerNo = primerNo;
          pF[outP++].name=ap.names[primerNo];
        }
      } if(didTruncate) {
        if(!doneTruncate) {
          fputs(truncateMsg,stderr); doneTruncate = 1; }
        fprintf(stderr,"%s\n",ap.names[primerNo]);
      }
    }
  } fflush(stderr);
  minValid=~(bit64)0; int i;
  for(i=0; i<nToFind; i++) {
    if(pF[i].valid < minValid) {
      minValid = pF[i].valid;
    }
  }
  for(i=0; i<nToFind; i++)
    pF[i].lhsBases = pF[i].p & minValid;
  qsort(pF,nToFind,sizeof(PrimerToFind),PF_cmp_func);
  *nRet = nToFind; return pF;
}

static char reportFilename[30]={0};
static char* getReportFilename() {
  /* Find a filename that hasn't yet been used.  TODO: proper locking semantics in case someone runs multiple instances in the same directory at the same time */
  int i=1; while(1) {
    snprintf(reportFilename,sizeof(reportFilename),"overlap-report-%d.txt",i++);
    FILE *f=fopen(reportFilename,"r");
    if(f) fclose(f); else break;
  }
  return reportFilename;
}
static FILE* checkPF(int nPrimers) {
  qsort(primerVariantsToFind,nPrimerVariantsToFind,sizeof(PrimerToFind),PF_by_didFind);
  FILE *reportFile = NULL;
  int i,nFound=0;
  for(i=0; i<nPrimers; i++) {
    if(didFind[i]) nFound++;
  }
  #ifndef Debug_ChromosomeCheck
  /* (no point printing all if reading 1 chromosome) */
  int reported=0;
  for(i=0; i<nPrimerVariantsToFind; i++) {
    if(!didFind[primerVariantsToFind[i].primerNo]) {
      if(!reported) {
        reported = 1; fputs("The following primers were not found in the genome:\n",stderr);
        reportFile = fopen(getReportFilename(),"w");
        if(reportFile) fputs("The following primers were not found in the genome:\n",reportFile);
        /* Previously reported number found more than once
           as well, but this turned out not to be useful
           because some primers can occur frequently but
           not in their amplicons.  We could make didFind
           boolean rather than a counter. */
      }
      fprintf(stderr,"%s\n",primerVariantsToFind[i].name);
      if(reportFile) fprintf(reportFile,"%s\n",primerVariantsToFind[i].name);
      didFind[primerVariantsToFind[i].primerNo] = 1; /* so we don't report this one a second time (as multiple entries in primerVariantsToFind map to the same primerNo) */
    }
  }
  #endif
  if (nFound == nPrimers) {
  fprintf(stderr,"All %d primers were found in the genome\n",nPrimers);
  if(reportFile) fprintf(reportFile,"All %d primers were found in the genome\n",nPrimers);
  } else {
  fprintf(stderr,"%d of %d primers were found in the genome\n",nFound,nPrimers);
  if(reportFile) fprintf(reportFile,"%d of %d primers were found in the genome\n",nFound,nPrimers);
  }
  return reportFile;
}

static int eventOrder(const void *aP, const void *bP) {
  /* <0 if A comes first;
     can't just subtract as we're unsigned */
  if(((AmpEvent*)aP)->baseEnd < ((AmpEvent*)bP)->baseEnd)
    return -1;
  return ((AmpEvent*)aP)->baseEnd > ((AmpEvent*)bP)->baseEnd;
}
static int findEndEvent(int seq,int startEvent,int maxAmpliconLen) {
  AmpEvent *events = eventLists[seq].events;
  #ifdef Debug_AmpliconNo
  if(Debug_AmpliconNo(events[startEvent].ampNo) && maxAmpliconLen)
    fprintf(stderr,"findEndEvent (%d/%d): baseStart=%u so max baseEnd=%u\n",events[startEvent].ampNo,events[startEvent].onOrOff,events[startEvent].baseStart,events[startEvent].baseStart+maxAmpliconLen);
  #endif
  int i; for(i=startEvent+1; i<(int)eventLists[seq].ptr; i++) {
    #ifdef Debug_AmpliconNo
    if(Debug_AmpliconNo(events[startEvent].ampNo))
      fprintf(stderr," - checking #%d: %d/%d (ends %u)\n",i,events[i].ampNo,events[i].onOrOff,events[i].baseEnd);
    #endif
    if(maxAmpliconLen && events[startEvent].baseStart+(b32)maxAmpliconLen < events[i].baseEnd) break; /* gone too far ahead */
    if(events[i].ampNo==events[startEvent].ampNo) {
      if (events[i].onOrOff == -events[startEvent].onOrOff) return i; /* found it */
      else break; /* duplicate start??  abort now and take second (shorter) one (v1.35 added) */
    }
  }
  #ifdef Debug_AmpliconNo
  if(Debug_AmpliconNo(events[startEvent].ampNo))
    fprintf(stderr," - not found (lookahead=%d)\n",i-startEvent);
  #endif
  return 0;
}
static char* eventsToOverlaps(int nAmp,int maxAmpliconLen,SeqName *names,FILE* *reportFileP,AllPrimers ap,const int *ampliconNoToFwd,const int *ampliconNoToRev,FILE *allAmps,int allAmpsIsMultiPLX,FILE *genome) {
  char *ampsFound=calloc(1,nAmp);
  char *overlaps=calloc(nAmp,nAmp); /* TODO: use triangle.h instead? */
  int* inProgress=malloc(nAmp*sizeof(int));
  int* inProgressI=malloc(nAmp*sizeof(int));
  if(memFail(ampsFound,overlaps,inProgress,inProgressI,_memFail)) return NULL;
  int nOverlaps = 0;
  if (allAmpsIsMultiPLX) addTags(ap); // prior to v1.33 this was incorrectly placed below the start of the seqNo loop, resulting in additional copies of tags being added for each new chromosome (usually overflowing the selected bit size so you get only the last part of the 2nd tag)
  int maxLenFound = 0;
  int seqNo; for(seqNo=0; seqNo<numEventLists; seqNo++) {
  memset(inProgress,0,nAmp*sizeof(int));
  memset(inProgressI,0,nAmp*sizeof(int));
  AmpEvent *events=eventLists[seqNo].events;
  qsort(events,eventLists[seqNo].ptr,sizeof(AmpEvent),eventOrder);
  int i; for(i=0; i<(int)eventLists[seqNo].ptr; i++) {
    int onOrOff = events[i].onOrOff,
      ampNo = events[i].ampNo;
    if(onOrOff < 0) { /* it's an end event */
      if(inProgress[ampNo] & -onOrOff)
        inProgress[ampNo] += onOrOff;
    } else if(inProgress[ampNo] & onOrOff) {
      /* already in progress: ignore */
    } else {
      int end=findEndEvent(seqNo,i,maxAmpliconLen);
      if(!end) continue;
      ampsFound[ampNo] = 1;
      inProgress[ampNo] += onOrOff;
      inProgressI[ampNo] = i;
      int ampLength = events[end].baseEnd+1-events[i].baseStart;
      if (ampLength>maxLenFound) maxLenFound=ampLength;
      if(allAmps) {
        if (allAmpsIsMultiPLX) {
          /* TODO: what if duplicate ampsFound[ampNo] ? */
          fprintf(allAmps,"%.*s",(int)strlen(events[i].name)-1-(strchr("-_",events[i].name[strlen(events[i].name)-2])!=0),events[i].name);
          fputc('\t',allAmps);
          printBasesMaybeD(ap,ampliconNoToFwd[events[i].ampNo],allAmps);
          fputc('\t',allAmps);
          printBasesMaybeD(ap,ampliconNoToRev[events[i].ampNo],allAmps);
          fputc('\t',allAmps);
          output_genome_segment(genome,seqNo,events[i].baseStart,ampLength,allAmps);
          fputc('\n',allAmps);
        } else fprintf(allAmps,"%s:%s (%s:%u%c%u)\n",events[i].name,events[end].name,&(names[seqNo][0]),events[i].baseStart,((onOrOff==1)?'+':'-'),events[end].baseEnd);
      }
      /* now check if there's any overlaps with other amplicons that are already running */
      int prnOver=0, j;
      for(j=0; j<nAmp; j++)
        if(j!=ampNo && inProgress[j] && !overlaps[ampNo*nAmp+j]) {
          overlaps[ampNo*nAmp+j] =
            overlaps[j*nAmp+ampNo] = 1;
          if(prnOver) fputs(", ",stderr); else {
            if(!nOverlaps) {
              fputs("Overlapping amplicons:\n",stderr);
              if(!*reportFileP) *reportFileP = fopen(getReportFilename(),"w");
              if(*reportFileP) fputs("Overlapping amplicons:\n",*reportFileP);
            }
            fprintf(stderr,"%s:%s (%s:%u%c%u) / ",events[i].name,events[end].name,&(names[seqNo][0]),events[i].baseStart,((onOrOff==1)?'+':'-'),events[end].baseEnd);
            if(*reportFileP) fprintf(*reportFileP,"%s:%s (%s:%u%c%u) / ",events[i].name,events[end].name,&(names[seqNo][0]),events[i].baseStart,((onOrOff==1)?'+':'-'),events[end].baseEnd);
            prnOver=1;
          }
          nOverlaps++;
          AmpEvent *overlapWithStart = events+inProgressI[j], *overlapWithEnd = events+findEndEvent(seqNo,inProgressI[j],maxAmpliconLen);
          fprintf(stderr,"%s:%s (%u%c%u)",overlapWithStart->name,overlapWithEnd->name,overlapWithStart->baseStart,((overlapWithStart->onOrOff==1)?'+':'-'),overlapWithEnd->baseEnd);
          if(*reportFileP) fprintf(*reportFileP,"%s:%s (%u%c%u)",overlapWithStart->name,overlapWithEnd->name,overlapWithStart->baseStart,((overlapWithStart->onOrOff==1)?'+':'-'),overlapWithEnd->baseEnd);
        }
      if(prnOver) {
        fputs("\n",stderr);
        if(*reportFileP) fputs("\n",*reportFileP);
      }
    }
  }} /* finished - rest of this function is reporting */
  if (allAmpsIsMultiPLX) removeTags(ap);
  int numNotFound = 0, i;
  for(i=0; i<nAmp; i++)
    if(!ampsFound[i]) {
      if(!numNotFound) {
        SetColour(Bright,Red,Black); fprintf(stderr,"Amplicons not found in genome:\n"); ResetColour();
        if(*reportFileP) fprintf(*reportFileP,"Amplicons not found in genome:\n");
      }
      numNotFound++;
      #ifdef Debug_AmpliconNo
      if(!Debug_AmpliconNo(i)) continue; /* don't let these get lost in the others */
      #endif
      fprintf(stderr,"%s/%s\n",ap.names[ampliconNoToFwd[i]],ap.names[ampliconNoToRev[i]]);
      if(*reportFileP) fprintf(*reportFileP,"%s/%s\n",ap.names[ampliconNoToFwd[i]],ap.names[ampliconNoToRev[i]]);
    }
  if(numNotFound) {
    SetColour(Bright,Red,Black); fprintf(stderr,maxAmpliconLen?"%d of %d amplicons not found in genome (with length <=%d)\n":"%d of %d amplicons not found in genome\n",numNotFound,nAmp,maxAmpliconLen); ResetColour();
    if(*reportFileP) fprintf(*reportFileP,maxAmpliconLen?"%d of %d amplicons not found in genome (with length <=%d)\n":"%d of %d amplicons not found in genome\n",numNotFound,nAmp,maxAmpliconLen);
  } else { SetColour(Dark,Green,Black); fputs("All amplicons were found in the genome\n",stderr); ResetColour(); } free(ampsFound);
  fprintf(stderr,"Longest amplicon found: %d bp\n",maxLenFound);
  if(maxLenFound >= 100000 /* 50000 was longest on record and very difficult to achieve */) fprintf(stderr,"  - this might include coincidental matches on other chromosomes;\n    try setting a sensible --amp-max\n"); /* relevant only if we're running on --amp-max=0 (v1.35+) */
  SetColour(Bright,Cyan,Black); fprintf(stderr,"%d overlaps found\n",nOverlaps); ResetColour();
  if(*reportFileP) fprintf(*reportFileP,"%d overlaps found\n",nOverlaps);
  free(inProgress); free(inProgressI); return overlaps;
}

char* GetOverlappingAmplicons(AllPrimers ap,FILE *genome,int* *primerNoToAmpliconNo,int *nAmplicons,int maxAmpliconLen,FILE *allAmps,int allAmpsIsMultiPLX) {
  int *ampliconNoToFwd,*ampliconNoToRev;
  time_t start = time(NULL);
  int nAmp = amplicons_from_primer_names(ap,primerNoToAmpliconNo,&ampliconNoToFwd,&ampliconNoToRev);
  char *overlaps = NULL;
  if(nAmp) {
    primerVariantsToFind = populatePF(ap,ampliconNoToFwd,ampliconNoToRev,nAmp,&nPrimerVariantsToFind);
    didFind=calloc(ap.np,sizeof(int));
    if(memFail(didFind,primerVariantsToFind,*primerNoToAmpliconNo,ampliconNoToFwd,ampliconNoToRev,_memFail)) return NULL;
    SeqName *names=go_through_genome(genome);
    FILE *reportFile = checkPF(ap.np);
    int found_primer_problems = (reportFile != NULL);
    overlaps = eventsToOverlaps(nAmp,maxAmpliconLen,names,&reportFile,ap,ampliconNoToFwd,ampliconNoToRev,allAmps,allAmpsIsMultiPLX,genome);
    if(reportFile) {
      fclose(reportFile);
      fprintf(stderr,"This report has also been written to %s",reportFilename);
      /* and re-print the time in case they didn't see it above (long overlap list) */ prnSeconds((long)(time(NULL)-start)); fputs("\n",stderr);
      if(found_primer_problems) { SetColour(Bright,White,Red); fputs("SOME PRIMERS WERE NOT FOUND: see start of the above.\n",stderr); ResetColour(); fputs("Possible causes:\n  - Mistakes in the primer file?\n  - Wrong reference genome?\n  - Trying to use mRNA primers on a complete genome?\n    (If using mRNA, you'll need an exon-only version of the genome)\n",stderr); }
      #ifndef Debug_ChromosomeCheck
      else { SetColour(Dark,Green,Black); fputs("(All primers were found in the genome.)\n",stderr); ResetColour(); }
      /* (not necessarily if we skipped the fopen because of Debug_ChromosomeCheck) */
      #endif
    }
    free(primerVariantsToFind);free(names);free(didFind);
    free(ampliconNoToFwd); free(ampliconNoToRev);
    free(eventLists);
  } *nAmplicons = nAmp; fclose(genome);
  if(allAmps && allAmps!=stdout) fclose(allAmps);
  return overlaps;
}
