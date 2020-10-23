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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <signal.h>
#define NDEBUG
#include <assert.h>
#include "openmp.h"
#include "all-primers.h"
#include "triangle.h"
#include "numbers.h"
#include "memcheck.h"

#define USE_QSORT 0 /* set this to 1 if you want to qsort the moves and pick either the best or next-best at random; 0 (faster) to take only the best move.  (TODO: user configurable?) */
#define PARALLELIZE_POOLSPLIT 1 /* having this at 1 can help find solutions more quickly (starts with a different initial randomization on each core)... */
#define PARALLELIZE_BESTMOVE 0 /* ...but setting this to 1 can make things SLOWER :-( unless dealing with a HUGE number of primer PAIRS (say, >2000 primers for >1000 pairs), otherwise the loop is too small for the parallelisation speedup to outweigh the thread-setup overhead */

/* Proposed move N = move primer N/(nPools-1) into pool currentPool + 1 + (N % (nPools-1)) (mod nPools).
   (Therefore every move MOVES something; no 'no-op's, since 1+(m mod (range-1)) cannot express going back to start)
   Immediate value = badnessContrib(cur) - badnessC(dest)
   We could save some division in qsort's inner loop by
   maintaing a separate array, but would need to profile
   as might be outweighed by worse locality of reference.
   These variables need to be available at module scope so
   that our qsort comparison function can get at them:
 */
#if USE_QSORT
static int *qsort_pools, qsort_nPools,
  *qsort_poolCounts, qsort_maxCount;
static ULL *qsort_bContrib;
/* otherwise we WON'T have them global, and we can be more
   multi-threaded w/out having to stall every time we call
   qsort (unless hard-code a pool of comparison funcs).
   The qsort version is slower ANYWAY and off by default,
   so I'm not too worried about having to stall for it. */
#endif

/* badness as ULL:
   bits 63-48: unsigned short maxScore (or invalid)
   bits 47-32: unsigned short num with this score
   bits 31-16: unsigned short num with this score - 1
   bits 15-0:  unsigned short num with this score - 2
   Note: bit 63 is always 0, so it's safe to cast to LL
   and do a subtraction to compare which is better of two
   states for qsort.  But 'value of move' is NOT equal to
   the DISTANCE between two of these; use valueOfReduction
*/
typedef unsigned short US;
enum { InvalidCombination=0x4000 }; /* (also mentioned in 64.h etc) If score is real score, 129 should do if no primers can be that long in this program.  But we'll need it higher if score is really -dG*10 (it's -dG*2 at the moment).  Don't approach 0x8000 though, + allow room for possible overflow of next field (so no 0x7FFF), although subtractBadness / valueOfReduction might need attention if we actually come to expect overflows between fields */
static inline int maxScoreOfBadness(ULL badness) {
  return (int)(badness>>48);
}
static inline void updateBadness(ULL *badness,int score) {
  int max = maxScoreOfBadness(*badness);
  assert(max <= InvalidCombination); assert(score <= InvalidCombination);
  if (score > max) {
    if (score > max+2) {
      *badness = ((ULL)score << 48) | ((ULL)1 << 32);
      return;
    } else while(score > max) { /* (TODO: could just write out the 2 cases of this loop, and change the below shifts into AND and add, IF profiling shows this needs it) */
        *badness = ((*badness >> 16) & 0xFFFFFFFFLL) | ((((*badness)>>48)+1)<<48); max++;
      }
  } /* now score <= max */
  int lswScore = max - 2;
  if(score < lswScore) return; /* this score is too insignificant to count at the current maximum level */
  int sL = (score-lswScore)*16;
  if(((*badness>>sL)&0xFFFF)==0xFFFF) return; /* saturated */
  *badness += ((ULL)1 << sL);
  assert(maxScoreOfBadness(*badness) == max);
}
static inline int subtractBadness(ULL *badness,int score) {
  /* for incremental updates.  Assume score has previously
     been included in updateBadness, so we don't have to
     worry about crossing 0 here.  */
  int max = maxScoreOfBadness(*badness);
  assert(score <= max);
  int lswScore = max - 2;
  if(score < lswScore) return 0; /* this score is too insignificant to affect the counters */
  int sL = (score-lswScore)*16;
  if(((*badness>>sL)&0xFFFF)==0xFFFF) return 0; /* if it was saturated, we'll have to leave it "stuck" there I'm afraid (unless return 1 to recalculate, but in many cases it would just saturate again) */
  *badness -= ((ULL)1 << sL);
  return !((*badness>>32)&0xFFFF); /* recalc max if count(max)==0 */
}
static inline ULL valueOfReduction(ULL from,ULL to) {
  /* for qsort etc.  We COULD return negative values as
     LL, but that would be more computation and we might
     as well just return 0, since we won't perform any
     value-based moves that don't have positive reductions
  */
  if(to > from) return 0;
  if(!to) return from;
  if(maxScoreOfBadness(from) == maxScoreOfBadness(to)) {
    /* 'from-to' will be a 48-bit value, and if get here
       then the high 16 bits of 'to' will be <= 'from',
       but if the mid 16 bits are >= then we need to set
       all of the low 32 bits to 0, and if the bottom
       16 bits are >= then we need to set low 16 to 0.  */
    ULL hi = (from & ((ULL)0xFFFF<<32)) - (to & ((ULL)0xFFFF<<32)), /* do NOT factor out the & part! */
      mid1 = from & ((ULL)0xFFFF<<16),
      mid2 = to & ((ULL)0xFFFF<<16);
    if (mid2 > mid1) return hi;
    ULL lo1 = from & 0xFFFF, lo2 = to & 0xFFFF;
    if (lo2 > lo1) return hi | (mid1-mid2);
    return from-to;
  }
  /* maxScoreOfBadness(from) > maxScoreOfBadness(to) :
     at the very least we need to 0-out the bottom 48 bits
     (TODO: we might be able to add some back in if
     maxScoreOfBadness(from) <= maxScoreOfBadness(to)+2,
     but probably OK just to do this for now...)
  */ return (from & ((ULL)0xFFFF<<48)) - (to & ((ULL)0xFFFF<<48));
}

static inline int primerOfMove(int m,int nPools) {
  return m/(nPools-1); }
static inline int oldPoolOfMove(int m,int nPools,const int* pools) {
  return pools[primerOfMove(m,nPools)]; }
static inline int poolOfMove(int m,int nPools,const int* pools) { return (oldPoolOfMove(m,nPools,pools)+1+(m % (nPools-1))) % nPools; }
static inline int primerAndDest_to_moveNo(int primer,int newPool,int nPools,const int *pools) {
  // works only if newPool != current pool
  return ((newPool+nPools-pools[primer]-1) % nPools) + primer*(nPools-1);
}
static inline int primerAndPool_to_contribOffset(int primer,int pool,int nPools) {
  return primer * nPools + pool; }
static inline int move_to_contribOffset(int m,int nPools,const int* pools) {
  return primerAndPool_to_contribOffset(primerOfMove(m,nPools),poolOfMove(m,nPools,pools),nPools); }

static inline ULL valueOfMove(int m,int nPools,const int* pools,const ULL*bContrib,const int *poolCounts,int maxCount) {
  if(maxCount && poolCounts[poolOfMove(m,nPools,pools)]==maxCount) return 0;
  assert(!maxCount || poolCounts[poolOfMove(m,nPools,pools)]<maxCount);
  ULL from = bContrib[primerAndPool_to_contribOffset(primerOfMove(m,nPools),oldPoolOfMove(m,nPools,pools),nPools)], /* what this primer was contributing to its old pool */
    to = bContrib[move_to_contribOffset(m,nPools,pools)]; /* what this primer will contribute to its new pool */
  assert(!to || poolCounts[poolOfMove(m,nPools,pools)]);
  return valueOfReduction(from,to);
}
#if USE_QSORT
static int betterMoves1st(const void *aP, const void *bP){
  int a = *(int*)aP, b = *(int*)bP;
  ULL vA = valueOfMove(a,qsort_nPools,qsort_pools,qsort_bContrib,qsort_poolCounts,qsort_maxCount), vB = valueOfMove(b,qsort_nPools,qsort_pools,qsort_bContrib,qsort_poolCounts,qsort_maxCount);
  /* Can't just subtract as it'll overflow an int */
  if (vA > vB) return -1; /* A is better, put first */
  else if (vA == vB) return 0;
  else return 1;
}
#else
static int findBestMove(const int *moves,int numMoves,int nPools,const int* pools,const ULL*bContrib,const int*poolCounts,int maxCount) {
  int bestMove = moves[0];
  ULL bestVal = valueOfMove(bestMove,nPools,pools,bContrib,poolCounts,maxCount);
  #if PARALLELIZE_BESTMOVE && defined(_OPENMP)
  #pragma omp parallel
  #endif
  {
    int priv_bestMove = bestMove;
    ULL priv_bestVal = bestVal;
    int i;
    #if PARALLELIZE_BESTMOVE && defined(_OPENMP)
    #pragma omp for schedule(static)
    #endif
    for(i=1; i<numMoves; i++) {
      ULL thisVal = valueOfMove(moves[i],nPools,pools,bContrib,poolCounts,maxCount);
      if(thisVal > priv_bestVal) {
        priv_bestVal = thisVal;
        priv_bestMove = moves[i];
      }
    }
    if(priv_bestVal > bestVal) {
      #if PARALLELIZE_BESTMOVE && defined(_OPENMP)
      #pragma omp critical
      #endif
      if (priv_bestVal > bestVal) {
        bestVal = priv_bestVal;
        bestMove = priv_bestMove;
      }
    }
  } return bestMove;
}
#endif

/* and here is code to set up & maintain that bContrib: */
static void badnessContrib(int primer,const int *scores,int np,int nPools,const int *pools,ULL *bContrib) {
  /* Assuming proposedPools[0:nPools] == 0 on entry, set proposedPools[0:nPools] to answer the Q: What contribution to the overall "badness" would primer make, assuming it were moved to (or left as-is in) proposedPools[n] and no other changes were made? */
  assert(primer>=0 && primer<np);
  ULL *proposedPools = bContrib + primer*nPools;
  int i;
  for(i=0; i<primer; i++) { /* ( <primer , primer ) */
    int pool = pools[i]; assert(pool>=0 && pool<nPools);
    updateBadness(proposedPools+pool,scores[primer-i]); /* if we put 'primer' in the same pool as 'i' is, we'll get the badness of the interaction between i and primer */
    scores += (np-i);
  }
  ++scores; /* i==primer, ignore interaction w. itself */
  for(++i; i<np; i++,scores++) { /* ( primer, >primer ) */
    int pool = pools[i]; assert(pool>=0 && pool<nPools);
    updateBadness(proposedPools+pool,*scores);
  }
}
static void badnessContribUpdate(int primer,const int *scores,int np,int otherPrimer,int otherOldPool,int nPools,const int *pools,ULL *bContrib,const int *poolCounts) {
  /* as above but just incrementally update primer's proposedPools in light of the fact that otherPrimer has just moved from otherOldPool to its current pool */
  assert(primer>=0 && primer<np && otherPrimer>=0 && otherPrimer<np && otherOldPool>=0 && otherOldPool<nPools);
  ULL *proposedPools = bContrib + primer*nPools;
  int  s, otherNewPool = pools[otherPrimer];
  assert(otherNewPool>=0 && otherNewPool<nPools);
  if(otherPrimer == primer) return;
  s = scores[t_offset(np,primer,otherPrimer)]; /* the score-contribution of interaction between primer and otherPrimer */
  if(!poolCounts[otherOldPool])
    proposedPools[otherOldPool] = 0; /* like the loop below but also clears any saturation (since we know there won't be saturation if the pool was left empty) */
  else if(subtractBadness(proposedPools+otherOldPool,s)) {
    /* oops, need to recalc the max of otherOldPool */
    proposedPools[otherOldPool] = 0;
    int i;
    for(i=0; i<primer; i++) { /* ( <primer , primer ) */
      if(pools[i] == otherOldPool)
        updateBadness(proposedPools+otherOldPool,scores[primer-i]);
      scores += (np-i);
    }
    ++scores;
    for(++i; i<np; i++,scores++) {
      if(pools[i]==otherOldPool)
        updateBadness(proposedPools+otherOldPool,*scores);
    }
  }
  updateBadness(proposedPools+otherNewPool,s);
}
static ULL globalBadness(const int *score,int np,const int *pools) {
  /* Measure across all primers in all pools.  We might
     perhaps be able to optimise this by making use of
     what we've already calculated in bContrib, but this
     function is called only when we get to local maxima.
  */
  int i,j; ULL m=0;
  for(i=0; i<np; i++) for(j=i; j<np; j++) if(pools[i]==pools[j]) updateBadness(&m,*score++); else score++;
  return m;
}

 static inline void make_a_move(int m,int np,const int *scores,const int *primerMove_depends_on,int nPools,int *pools,ULL *bContrib,int *poolCounts,int maxCount) {
  int primer = primerOfMove(m,nPools),
    oldPool = oldPoolOfMove(m,nPools,pools),
    newPool = poolOfMove(m,nPools,pools);
  assert(primer >= 0 && primer < np && oldPool>=0 && newPool>=0 && oldPool<nPools && newPool<nPools && oldPool != newPool);
  pools[primer] = newPool; /* 'm' changes meaning now */
  assert(poolCounts[oldPool]);
  poolCounts[oldPool]--; poolCounts[newPool]++;
  assert(!maxCount || poolCounts[newPool]<=maxCount);
  int i; for(i=0; i<np; i++) {
    if(primerMove_depends_on[i]==primer)
      pools[i] = newPool; /* see merge_scores_of_stuckTogether_primers (and DON'T need to update poolCounts here) */
    else badnessContribUpdate(i,scores,np,primer,oldPool,nPools,pools,bContrib,poolCounts);
  }
}

static inline int should_stick_together(AllPrimers ap,int i,int j) {
  /* Names same except last letter = keep in same pool */
  const char *n1=ap.names[i], *n2=ap.names[j];
  size_t l1=strlen(n1),l2=strlen(n2);
  return (l1 == l2 && !strncmp(n1,n2,l1-1)); /* TODO: case-insensitive? */
}
static inline void updateMax(int *i,int m) {
  if(m>*i) *i=m;
}
static int* merge_scores_of_stuckTogether_primers(AllPrimers ap,int *scores) {
  int i,j,*p=scores; if(!p) return NULL;
  int *primerMove_depends_on=malloc(ap.np*sizeof(int));
  if(!primerMove_depends_on) return NULL;
  memset(primerMove_depends_on,0xFF,ap.np*sizeof(int));
  char *pairedOK=malloc(ap.np);
  if(!pairedOK) {
    free(primerMove_depends_on); return NULL;
  }
  memset(pairedOK,0,ap.np);
  int doneMerge = 0;
  for(i=0; i<ap.np; i++) for(j=i; j<ap.np; j++) {
      if(i!=j && primerMove_depends_on[i]==-1 && primerMove_depends_on[j]==-1 && should_stick_together(ap,i,j)) {
        /* For simplicity of pooling, we'll set it so:
           - Interactions with i get maxed with those w.j
           - Interactions with j itself "don't count"
           - j is not allowed to be moved by itself
           - j is always moved when i moves */
        *p = 0; /* so S(i,j) = 0 */
        int k,*kp=scores; /* max S(k,i) with S(k,j): */
        int *Sip=0; /* =0 to suppress compiler warning */
        for(k=0; k<j; k++) {
          if(k<i) {
            updateMax(kp+i-k,kp[j-k]); kp[j-k]=0;
          } else if(k==i) {
            Sip = kp+1; /* needed for S(i,k) */
          } else { /* max S(i,k) with S(k,j) */
            updateMax(Sip++,kp[j-k]); kp[j-k]=0;
          }
          kp += (ap.np-k);
        } k++; kp++; Sip++; /* ignore k==j */
        for(;k<ap.np;k++) {
          /* max S(i,k) [=Sip] with S(j,k) [=kp] */
          updateMax(Sip++,*kp); *kp++=0;
        }
        primerMove_depends_on[j] = i;
        doneMerge = pairedOK[i] = pairedOK[j] = 1;
      }
      p++;
    }
  if(doneMerge) {
    /* just check for lone primers, usually a bad sign */
    for(i=0; i<ap.np; i++) if(!pairedOK[i]) fprintf(stderr,"Warning: ungrouped primer %s\n",ap.names[i]);
  } else {
    /* same message as in amplicons.c (see comment there)
       in case overlap-check was missed */
    fputs("WARNING: No primers are paired!\nPlease end your forward primers with -F\nand your reverse primers with -R or -B as instructed\n",stderr);
  }
  free(pairedOK); return primerMove_depends_on;
}

static inline int should_stick_to_pool(AllPrimers ap,int i) {
  /* if the user wants some primers to be fixed to
     specific pools (and we move the rest around) */
  const char *n=ap.names[i];
  if(*n == '@' && *(++n)>='0' && *n<='9') {
    char *end; int pool=(int)strtol(n,&end,10);
    if(*end==':') {
      /* we have a valid @<pool number>: */
      return pool-1; /* (internally start at 0) */
    }
  }
  return -1;
}
static int* pre_fix_primers_to_pools(AllPrimers ap) {
  int *fix_to_pool=malloc(ap.np*sizeof(int)), i;
  if(!fix_to_pool) return NULL;
  for(i=0; i<ap.np; i++)
    fix_to_pool[i] = should_stick_to_pool(ap,i);
  return fix_to_pool;
}

static void saturate_scores_of_overlapping_primers(int *scores,const char *overlappingAmplicons,const int *primerNoToAmpliconNo,int nAmplicons,int np) {
  int i,j,*p=scores; if(!p || !nAmplicons) return;
  assert(overlappingAmplicons);
  for(i=0; i<np; i++) for(j=i; j<np; j++) {
      assert(*p<InvalidCombination);
      if(i!=j && primerNoToAmpliconNo[i]!=-1 && primerNoToAmpliconNo[j]!=-1 && overlappingAmplicons[primerNoToAmpliconNo[i]*nAmplicons+primerNoToAmpliconNo[j]])
        *p = InvalidCombination;
      p++;
    }
}

static void printNumInEachPool(const int *poolCounts,int numPools) {
  fprintf(stderr,"\tPool sizes: "); int i;
  for(i=0; i<numPools; i++) {
    if(i) fprintf(stderr,"|");
    fprintf(stderr,"%d",poolCounts[i] << 1); /* TODO: this "<< 1" assumes countOf(primerMove_depends_on==-1) == np/2, but that is almost certainly going to be the case, unless somebody is doing something very strange, and if the worst comes to the worst it's only an informational pool-size display going a bit wrong */
  } fprintf(stderr,"\n");
}

static int IntCompare(const void *a,const void *b) {
  return *(const int*)b-*(const int*)a;
}
static int* numInEachPool(const int *pools,int np,int numPools,const int *primerMove_depends_on) {
  /* for after everything has finished and the per-thread poolCounts has been freed */
  int* counts=calloc(numPools,sizeof(int));
  if(!counts) return NULL;
  int i; for(i=0; i<np; i++)
           if(primerMove_depends_on[i]==-1)
             counts[pools[i]]++;
  qsort(counts,numPools,sizeof(int),IntCompare);
  return counts;
}

enum { s_KeepGoing = 0, s_ccPressed, s_tooManyIters };
static volatile int stop_state;
static void intHandler(int s) { stop_state = s_ccPressed; }

static void randomise_pools(int np,const int *primerMove_depends_on,const int *fix_to_pool,const int *scores,int nPools,int *pools,ULL *bContrib,int *poolCounts,int maxCount) {
  /* initialise to random distribution of pools, but note
     primerMove_depends_on and maxCount when doing this.
     Also initialise bContrib.  */
  int i; memset(poolCounts,0,nPools*sizeof(int));
  /* First set all fixed-pool primers in place,
     before randomising the others around them */
  for(i=0; i<np; i++)
    if(primerMove_depends_on[i] == -1) {
      int pool = fix_to_pool[i];
      if(pool != -1) {
        if(maxCount && poolCounts[pool]==maxCount && !(maxCount==1 && nPools==np)) {
          /* (last part of that condition detects call by suggest_num_pools,
             where it's OK if fixed-pool primers make us exceed 1 per pool) */
          fprintf(stderr, "randomise_pools ERROR: maxCount too small for fixed primer in pool %d\n",fix_to_pool[i]);
          abort();
        } pools[i]=pool; poolCounts[pool]++;
      }
    }
  for(i=0; i<np; i++)
    if(primerMove_depends_on[i] == -1 && fix_to_pool[i] == -1) {
      int pool = ThreadRand() % nPools;
      int origPool = pool;
      while(maxCount && poolCounts[pool]>=maxCount) {
        pool++; /* not very random but it'll do for now */
        if(pool==nPools) pool=0;
        if(pool==origPool) {
          fprintf(stderr, "randomise_pools ERROR: maxCount too small, can't fit\n");
          abort();
        }
      } pools[i]=pool; poolCounts[pool]++;
    }
  for(i=0; i<np; i++)
    if(primerMove_depends_on[i]>-1)
      /* DON'T update poolCounts here (it's in pairs so moveTooLopsided doesn't have to account for this one) */
      pools[i]=pools[primerMove_depends_on[i]];
  memset(bContrib,0,np*nPools*sizeof(ULL));
  for(i=0; i<np; i++) badnessContrib(i,scores,np,nPools,pools,bContrib);
}

static int* initMoves(int *numMoves,int np,int nPools,const int *primerMove_depends_on,const int *fix_to_pool) {
  if(nPools <= 1) return NULL;
  int *moves=malloc(np*(nPools-1)*sizeof(int));
  if(moves) {
    int *movesP = moves, i;
    for(i=0; i<np*(nPools-1); i++) {
      int primer = primerOfMove(i,nPools);
      if(primerMove_depends_on[primer]==-1
         && fix_to_pool[primer]==-1) *movesP++ = i;
    }
    *numMoves = movesP-moves;
    moves = memTrim(moves,movesP);
  } return moves;
}

#if Has_128bit
typedef bit128 ThreadMask;
#else
typedef ULL ThreadMask;
#endif

static void poolsplit_thread(const int* shared_moves,AllPrimers ap,int nPools,int numMoves,const int* primerMove_depends_on,const int* fix_to_pool,const int* scores,time_t limitTime,int *bestPools,const float* table, int* bestPools_init_yet,ULL* gBadLast,long *totalIterations,time_t *lastOutputTime,int *overlaps,int* just_printed_counts,ThreadMask* threads_needing_to_reset_iter,int maxCount) {
  /* This is the inner part of split_into_pools.
     Multiple instances may be called in parallel. */
  int iter = 0, willContinue=1;
  int *moves = (int*)shared_moves;
  ULL *bContrib = malloc(ap.np*nPools*sizeof(ULL));
  int *poolCounts=malloc(nPools*sizeof(int));
  int *pools = NULL;
  if(memFail(bContrib,poolCounts,_memFail))
    willContinue = 0;
  else {
    pools = malloc(ap.np*sizeof(int));
    if(memFail(pools,_memFail)) willContinue = 0;
    else if(USE_QSORT) { /* moves must be per-thread */
      moves=malloc(numMoves*sizeof(int));
      if(memFail(moves,_memFail)) willContinue = 0;
      else wrapped_memcpy(moves,shared_moves,numMoves*sizeof(int));
    }
  }
  ThreadMask myMask = ((ThreadMask)1) << omp_get_thread_num(); /* for threads_needing_to_reset_iter */
  if (!myMask) {
    /* what, somebody's running us on >128 cores ?? (or >64 32-bit cores) */
    /* (versions below v1.16 would hit this after 32 cores and not detect it) */
    #if defined(_OPENMP)
    #pragma omp critical
    #endif
    fprintf(stderr,"Can't run thread number %d because ThreadMask type has only %d bits\n",omp_get_thread_num(),(int)sizeof(ThreadMask)*8);  /* If you hit this, I suggest you either find a wider ThreadMask type or else we'd better make it an array.  Haven't done it so far because I've tested only on a 4-core machine and I doubt the chances of being run on many more cores than that are particularly high in 2016 (future might be different) */
    willContinue = 0;
  }
  int max_iterations = 10000000 /* TODO: customise? profile? (but low priority as we have an interrupt mechanism) */
    / (omp_get_num_threads() > 10 ? 10 : omp_get_num_threads()); /* TODO: customise this "10" as well? (it's maxMoves / minMoves) */
  while(willContinue) {
    randomise_pools(ap.np,primerMove_depends_on,fix_to_pool,scores,nPools,pools,bContrib,poolCounts,maxCount);
    for(; ; iter++) {
      #if USE_QSORT
      #if PARALLELIZE_POOLSPLIT && defined(_OPENMP)
      #pragma omp critical
      #endif
      {
        qsort_pools = pools; qsort_nPools = nPools;
        qsort_bContrib = bContrib;
        qsort_poolCounts = poolCounts;
        qsort_maxCount = maxCount;
        qsort(moves,numMoves,sizeof(int),betterMoves1st);
      }
      int bestMove = moves[0];
      #else
      int bestMove = findBestMove(moves,numMoves,nPools,pools,bContrib,poolCounts,maxCount);
      #endif
      if(*threads_needing_to_reset_iter & myMask) {
        #if PARALLELIZE_POOLSPLIT && defined(_OPENMP)
        #pragma omp critical
        #endif
        {
          *threads_needing_to_reset_iter &= ~myMask;
          *totalIterations += iter;
        } iter = 0;
      }
      int timesUp = stop_state || (limitTime && time(NULL) >= limitTime);
      if(timesUp || !valueOfMove(bestMove,nPools,pools,bContrib,poolCounts,maxCount)) {
        /* looks like we're at a local maxima */
        willContinue = !timesUp;
        ULL gBad = globalBadness(scores,ap.np,pools);
        int keep = !*bestPools_init_yet || gBad < *gBadLast;
        if (keep)
          #if defined(_OPENMP)
          #pragma omp critical
          #endif
          if ((keep = !*bestPools_init_yet || gBad < *gBadLast) != 0) {
          *bestPools_init_yet = 1;
          wrapped_memcpy(bestPools,pools,ap.np*sizeof(int));
          *gBadLast = gBad; *totalIterations += iter;
        }
        if(gBad < (ULL)1<<48) willContinue=0; // everything down to score 0 - can't very much improve on that (except for reducing # pools or size difference)
        if (keep) {
          iter = 0;
          if(time(NULL)-*lastOutputTime > 2) {
            int should_print_counts = 0;
            #if PARALLELIZE_POOLSPLIT && defined(_OPENMP)
            #pragma omp critical
            #endif
            if(time(NULL)-*lastOutputTime > 2) {
              *lastOutputTime = time(NULL);
              *threads_needing_to_reset_iter = ~0;
              should_print_counts = 1;
            } if(should_print_counts) {
            *overlaps=table?dGprintPooledCounts(ap,pools,scores,stderr) : printPooledCounts(ap,pools,scores);
            printNumInEachPool(poolCounts,nPools);
            if(!willContinue) {
              *just_printed_counts = 1; break; }
            fprintf(stderr,"Local maxima found after %" QUOT "ld moves\nTrying to better it... (press Control-C to stop)\n",*totalIterations+iter); /* TODO: what about the 'iter' values of other threads? (or just don't count them yet) */
            fflush(stderr); /* in case of broken Windows/WINE etc (see comments in user.c) */
            }
          }
        } else {
          /* this maxima doesn't beat the best we've seen */
          if(iter>max_iterations && !stop_state) {
            fputs("Too many moves without improvement: giving up\n",stderr);
            willContinue=0; /* and stop other threads: */
            stop_state = s_tooManyIters;
          }
        }
        if(!willContinue) break;
        if(keep) {
          /* already found a good local maxima, so just take a few random steps away from it... */
          int randomMoves = 5+ThreadRand()%5, i;
          for(i=0; i<randomMoves; i++) {
            int moveToMake=ThreadRand()%numMoves;
            if(maxCount) while(poolCounts[poolOfMove(moves[moveToMake],nPools,pools)]==maxCount) if(++moveToMake==numMoves) moveToMake=0;
            make_a_move(moves[moveToMake],ap.np,scores,primerMove_depends_on,nPools,pools,bContrib,poolCounts,maxCount);
          } continue; /* don't do the additional make_a_move below (we'd have to repeat the maxCount condition) */
        } else {
          /* local maximae getting worse...
             get me out of here! */
          break;
        }
      }
      #if USE_QSORT
      int i = 0;
      while(!(ThreadRand()%5) && i<numMoves-1 && valueOfMove(moves[i+1],nPools,pools,bContrib,poolCounts,maxCount)) ++i; /* sometimes don't pick the best one, just in case (TODO: can we write code to "get the top N items" w/out a complete sort?) */
      bestMove = moves[i];
      #endif
      make_a_move(bestMove,ap.np,scores,primerMove_depends_on,nPools,pools,bContrib,poolCounts,maxCount);
    }
  }
  if(bContrib) free(bContrib);
  if(pools) free(pools);
  free(poolCounts);
  #if PARALLELIZE_POOLSPLIT && defined(_OPENMP)
  #pragma omp critical
  #endif
  *totalIterations += iter;
}

PS_cache PS_precalc(AllPrimers ap,const float *table,const char *overlappingAmplicons,const int *primerNoToAmpliconNo,int nAmplicons) {
  PS_cache r;
  addTags(ap);
  r.scores = table ? dGtriangle(ap,table) : triangle(ap);
  removeTags(ap);
  r.primerMove_depends_on = merge_scores_of_stuckTogether_primers(ap,r.scores);
  r.fix_to_pool = pre_fix_primers_to_pools(ap);
  if(memFail(r.scores,r.primerMove_depends_on,r.fix_to_pool,_memFail)) r.scores = NULL;
  else {
    saturate_scores_of_overlapping_primers(r.scores,overlappingAmplicons,primerNoToAmpliconNo,nAmplicons,ap.np);
    r.fix_min_pools = 2;
    int i; for(i=0; i<ap.np; i++) if(r.fix_to_pool[i]>=r.fix_min_pools) r.fix_min_pools=r.fix_to_pool[i]+1;
  }
  return r;
}
void PS_free(PS_cache c) {
  if(c.scores) {
    free(c.scores);
    free(c.primerMove_depends_on);
    free(c.fix_to_pool);
  }
}

int* split_into_pools(AllPrimers ap,int nPools,int timeLimit,PS_cache cache,int seedless,const float *table,int maxCount) {
  int *scores = cache.scores; if(!scores) return NULL;
  int *primerMove_depends_on = cache.primerMove_depends_on;
  int *fix_to_pool = cache.fix_to_pool;
  {
    if(nPools<cache.fix_min_pools) { fprintf(stderr,"ERROR: @%d:primers need at least %d pools, but only got %d\n",cache.fix_min_pools,cache.fix_min_pools,nPools); return NULL; }
  }
  if(maxCount) { int denom=0,i; for(i=0; i<ap.np; i++) if(primerMove_depends_on[i]!=-1) denom++; maxCount=maxCount*denom/ap.np; if(!maxCount) maxCount=1; } /* pairs */
  int numMoves=0,*shared_moves=initMoves(&numMoves,ap.np,nPools,primerMove_depends_on,fix_to_pool); /* =0 to stop warnings on old compilers */
  if(memFail(shared_moves,_memFail)) return NULL;
  if(!numMoves) {
    fputs("Can't move anything!\n",stderr);
    free(shared_moves); return NULL;
  }
  int *bestPools = malloc(ap.np*sizeof(int));
  if(memFail(shared_moves,bestPools,_memFail)) return NULL;
  time_t start = time(NULL);
  srand(seedless ? 1 : start);
  int bestPools_init_yet = 0; ULL gBadLast=0; /* latter =0 to stop warnings on old compilers */
  time_t lastOutputTime = (time_t)0; /* so 1st maxima gets output no matter what (might be needed if break after) */
  time_t limitTime = (time_t)0; if(timeLimit) limitTime = time(NULL) + timeLimit*60; /* (timeLimit is in minutes) */
  int just_printed_counts = 0, overlaps = 0;
  stop_state = s_KeepGoing; signal(SIGINT, intHandler);
  if(omp_get_max_threads() > 1) {
    if(seedless) { omp_set_num_threads(1); fputs("NOT parallelising the pool trials, as you asked for predictability.\n",stderr); }
    else fprintf(stderr,"Parallelising pool trials: %d threads\n",omp_get_max_threads());
  }
  fprintf(stderr,"OK, here goes... (press Control-C to stop%s)\n",timeLimit?" early":""); fflush(stderr);
  long totalIterations = 0;
  ThreadMask threads_needing_to_reset_iter = 0;
  #if PARALLELIZE_POOLSPLIT && defined(_OPENMP)
  #pragma omp parallel
  #endif
  poolsplit_thread(shared_moves,ap,nPools,numMoves,primerMove_depends_on,fix_to_pool,scores,limitTime,bestPools,table, &bestPools_init_yet,&gBadLast,&totalIterations,&lastOutputTime,&overlaps,&just_printed_counts,&threads_needing_to_reset_iter,maxCount);
  signal(SIGINT, SIG_DFL);
  if(!just_printed_counts) {
    fputs("... looks like this is the best I can do:\n",stderr);
    overlaps = table ? dGprintPooledCounts(ap,bestPools,scores,stderr) : printPooledCounts(ap,bestPools,scores);
    int *counts=numInEachPool(bestPools,ap.np,nPools,primerMove_depends_on);
    if(counts) {
      printNumInEachPool(counts,nPools);
      free(counts);
    }
  }
  long numSecs = (long)(time(NULL)-start);
  if(!numSecs) numSecs=1; /* so division doesn't crash */
  fprintf(stderr,"%" QUOT "ld moves",totalIterations);
  prnSeconds(numSecs); fprintf(stderr," = %" QUOT "ld/sec\n",totalIterations/numSecs);
  if(bestPools && overlaps) {
    if(stop_state == s_tooManyIters) fprintf(stderr,"WARNING: There are still overlaps in these pools,\neven after this number of moves.\nYou might need more pools.\n");
    else fprintf(stderr,"WARNING: There are still overlaps in these pools.\nMaybe you should have let it run longer\nto see if these overlaps can be eliminated.\n");
  } fflush(stderr);
  free(shared_moves); return bestPools;
}

int suggest_num_pools(AllPrimers ap,PS_cache cache,const float *table) {
  /* Apply a simple threshold-based allocation just for
     suggesting a number of pools */
  int threshold = table ? 14 : 7; /* dG -7 or score 7.  TODO: customise?  but this function is for when the user is not sure, so perhaps we'd best hard-code the threshold */
  int nPools = ap.np; /* worst case is none of the primers are paired (unpaired primers could hang randomise_pools before v1.42 because this line said ap.np/2) */
  int *scores = cache.scores; if(!scores) return 0;
  int *primerMove_depends_on = cache.primerMove_depends_on;
  int *fix_to_pool = cache.fix_to_pool;
  ULL *bContrib = malloc(ap.np*nPools*sizeof(ULL));
  int *poolCounts=malloc(nPools*sizeof(int));
  int *pools = malloc(ap.np*sizeof(int));
  if(memFail(bContrib,poolCounts,pools,_memFail))
    return 0;
  randomise_pools(ap.np,primerMove_depends_on,fix_to_pool,scores,nPools,pools,bContrib,poolCounts,1); /* puts 0 or 1 set in each pool (after the fixed ones) */
  int suggest_nPools = 1;
  int primer; for (primer=0; primer<ap.np; primer++) if (primerMove_depends_on[primer]==-1) {
      if (fix_to_pool[primer]==-1) {
        int destPool; for (destPool=0; destPool < suggest_nPools; destPool++) if(maxScoreOfBadness(bContrib[primerAndPool_to_contribOffset(primer,destPool,nPools)]) <= threshold) break; /* find first pool it will 'fit' in */
        if (destPool == suggest_nPools) suggest_nPools++;
        if (pools[primer] != destPool) make_a_move(primerAndDest_to_moveNo(primer,destPool,nPools,pools),ap.np,scores,primerMove_depends_on,nPools,pools,bContrib,poolCounts,ap.np);
      } else if (fix_to_pool[primer] >= suggest_nPools) {
        /* must have at least as many for the fixed-pool primers
         (and fix_to_pool starts numbering at 0, so +1 of course) */
        suggest_nPools = fix_to_pool[primer] + 1;
      }
  }
  free(bContrib); free(pools); free(poolCounts);
  return suggest_nPools;
}
