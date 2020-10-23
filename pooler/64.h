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
/* Anything in sixty four dot h calls sixty four bit funcs
   and 128.h is auto-generated: rplace w.128 throughout
   (similarly for 32.h for 32-bit architectures).

   These files should be included ONLY by bit-common.h.
   The general interface is in all-primers.h.
*/

typedef struct { bit64 AorT, GorT, valid; } Primer64;
typedef struct { bit64 MaybeA, MaybeC, MaybeG, MaybeT; } DegeneratePrimer64; /* more than one possibility for each base */
typedef struct {
  union {
    DegeneratePrimer64 D;
    Primer64 notD;
  } p;
  int isD;
} MaybeDegeneratePrimer64;
static MaybeDegeneratePrimer64 wrapPrimer64(Primer64 p) {
  MaybeDegeneratePrimer64 r;
  r.p.notD = p; r.isD = 0; return r;
}
static MaybeDegeneratePrimer64 wrapDegeneratePrimer64(DegeneratePrimer64 p) {
  MaybeDegeneratePrimer64 r;
  r.p.D = p; r.isD = 1; return r;
}
static inline DegeneratePrimer64 upgradeToDegenerate64(MaybeDegeneratePrimer64 p) {
  if(p.isD) return p.p.D;
  DegeneratePrimer64 d;
  Primer64 notD = p.p.notD;
  d.MaybeA = notD.valid & notD.AorT & ~notD.GorT;
  d.MaybeC = notD.valid & ~(notD.AorT | notD.GorT);
  d.MaybeG = notD.valid & ~notD.AorT & notD.GorT;
  d.MaybeT = notD.valid & notD.AorT & notD.GorT;
  return d;
}
static inline bit64 DegenerateValid64(DegeneratePrimer64 p) {
  /* returns the valid bits of p (we could store this separately, but it's redundant, and might be better pay-off to keep things small and cacheable) */
  return p.MaybeA | p.MaybeC | p.MaybeG | p.MaybeT;
}

static Primer64 parsePrimer64(const char *ACGT) {
  Primer64 r={0,0,0};
  while(1) {
    char l=fast_toUpper(*ACGT++);
    if(!l) return r;
    if(strchr("AGCT",l)) {
      r.AorT = (r.AorT<<1) | (l=='A'||l=='T');
      r.GorT = (r.GorT<<1) | (l=='G'||l=='T');
      r.valid = (r.valid<<1) | 1;
    } else reportUnrecognisedBase(l);
  }
}
static DegeneratePrimer64 parseDegeneratePrimer64(const char *ABC) {
  DegeneratePrimer64 r={0,0,0,0};
  while(1) {
    char l=fast_toUpper(*ABC++);
    if(!l) return r;
    const char *combo = strchr(degenerateCombos,l);
    if(combo) {
      int c=combo-degenerateCombos + 1;
      r.MaybeA = (r.MaybeA<<1) | ((c&8)!=0);
      r.MaybeC = (r.MaybeC<<1) | ((c&4)!=0);
      r.MaybeG = (r.MaybeG<<1) | ((c&2)!=0);
      r.MaybeT = (r.MaybeT<<1) | ((c&1)!=0);
    } else reportUnrecognisedBase(l);
  }
}
static MaybeDegeneratePrimer64 parseMaybeDegeneratePrimer64(const char *ABC) {
  size_t l=strcspn(ABC,"BDHKMNRSVWYbdhkmnrsvwy");
  if(ABC[l]) return wrapDegeneratePrimer64(parseDegeneratePrimer64(ABC));
  else return wrapPrimer64(parsePrimer64(ABC));
}

static inline void PrimerComplement64(Primer64 *p) {
  // A->T, T->A, C->G, G->C.  So AorT stays, GorT flipped.
  p->GorT = ~(p->GorT);
}
static inline void PrimerComplement64D(DegeneratePrimer64 *p) {
  bit64 tmp=p->MaybeA; p->MaybeA=p->MaybeT; p->MaybeT=tmp;
  tmp=p->MaybeG; p->MaybeG=p->MaybeC; p->MaybeC=tmp;
}
static inline void PrimerComplement64MaybeD(MaybeDegeneratePrimer64 *p) {
  if(p->isD) PrimerComplement64D(&(p->p.D));
  else PrimerComplement64(&(p->p.notD));
}

static int NumPossibilities64D_32bases(DegeneratePrimer64 d) {
  /* number of possible primers this degenerate primer might be equal to (assumes aligned right) */
  int r = 1, sR, poss;
  for(sR=0; sR<32; sR++) { /* 32, not sixty four etc, because we want the return value to work with Make2bitFrom64D  */
    poss = ((d.MaybeA >> sR) & 1) + ((d.MaybeC >> sR) & 1)
      + ((d.MaybeG >> sR) & 1) + ((d.MaybeT >> sR) & 1);
    if (poss) r *= poss; else break;
  }
  return r;
}
static int NumPossibilities64MaybeD_32bases(MaybeDegeneratePrimer64 d) {
  if(d.isD) return NumPossibilities64D_32bases(d.p.D);
  else return 1;
}
static int Make2bitFrom64D(DegeneratePrimer64 d,ULL *out,ULL *outValid,int possNo,int nPoss) {
  /* note: ULL across all 3 .h variants
     - more than 32 bases here will be truncated
     (return value is 1 if it got truncated).
     TODO: we could make a "not degenerate" version of
     this which just shifts bits around, but I'd be very
     surprised if this function is anywhere near the top
     of a profile trace, so for now I'll leave it as you
     have to call upgradeToDegenerate64 from the Maybe.

     Note: for ease of binary search (see amplicons.c),
     bases are shifted in from the LEFT (not from the
     right as in the other functions), and the result
     reads backwards from the 'end' cursor at left.
  */
  int sR, poss;
  ULL toOut=0,toOutValid=0;
  for(sR=0; sR<32; sR++) { /* 32, not sixty four etc */
    int MaybeA = ((d.MaybeA >> sR) & 1),
      MaybeC = ((d.MaybeC >> sR) & 1),
      MaybeG = ((d.MaybeG >> sR) & 1),
      MaybeT = ((d.MaybeT >> sR) & 1);
    poss = MaybeA + MaybeC + MaybeG + MaybeT;
    if (poss) {
      int partitionSize = nPoss / poss;
      int possToTake = possNo / partitionSize;
      possNo %= partitionSize; nPoss /= poss;
      ULL bits = 0; /* we set it to a value to stop the "might be uninitialised" warning */
      if (MaybeT) possToTake--; /* if(!possToTake--) bits=0; but it's at 0 anyway */
      if (MaybeC && !possToTake--) bits=1;
      if (MaybeA && !possToTake--) bits=2;
      if (MaybeG && !possToTake) bits=3;
      int sL = 62-2*sR; /* IMPORTANT: don't write (64-2) as it'll be changed to 32-2 in 32.h; this is ULL */
      toOut |= (bits << sL);
      toOutValid |= ((ULL)3 << sL);
    } else break;
  } *out=toOut; *outValid = toOutValid;
#define Is_64bit /* will change to Is_32bit in 32.h */
#ifdef Is_32bit
  return 0; // avoid compiler warnings
#else
  return ((d.MaybeA >> 32) | (d.MaybeC >> 32) | (d.MaybeG >> 32) | (d.MaybeT >> 32)) != 0;
#endif
}

static inline Primer64 PrimerReverse64(Primer64 p) {
  /* assumes 'valid' is right-aligned */
  bit64 v=p.valid,i1=p.AorT,i2=p.GorT,o1=0,o2=0;
  while(v) {
    o1=(o1<<1)|(i1&1); o2=(o2<<1)|(i2&1);
    i1>>=1; i2>>=1; v>>=1;
  }
  p.AorT = o1; p.GorT = o2; return p;
}
static inline DegeneratePrimer64 DegeneratePrimerReverse64(DegeneratePrimer64 p) {
  /* assumes right-aligned */
  bit64 i1=p.MaybeA, i2=p.MaybeC, i3=p.MaybeG, i4=p.MaybeT, o1=0, o2=0, o3=0, o4=0;
  while(i1 || i2 || i3 || i4) {
    o1=(o1<<1)|(i1&1); o2=(o2<<1)|(i2&1);
    o3=(o3<<1)|(i3&1); o4=(o4<<1)|(i4&1);
    i1>>=1; i2>>=1; i3>>=1; i4>>=1;
  }
  p.MaybeA = o1; p.MaybeC = o2; p.MaybeG = o3; p.MaybeT = o4; return p;
}
static inline MaybeDegeneratePrimer64 MaybeDegeneratePrimerReverse64(MaybeDegeneratePrimer64 p) {
  if(p.isD) return wrapDegeneratePrimer64(DegeneratePrimerReverse64(p.p.D));
  else return wrapPrimer64(PrimerReverse64(p.p.notD));
}

static inline void PrimerTag64(Primer64 *p,Primer64 tag) {
  /* assumes 'valid' is right-aligned in both p and tag */
  int sL = popcount64(p->valid); /* = 64-leading0_64 */
  p->valid |= (tag.valid << sL);
  p->AorT |= (tag.AorT << sL); p->GorT|=(tag.GorT << sL);
}
static inline void DegeneratePrimerTag64(DegeneratePrimer64 *p,DegeneratePrimer64 tag) {
  int sL = popcount64(p->MaybeA | p->MaybeC | p->MaybeG | p->MaybeT);
  p->MaybeA |= (tag.MaybeA << sL);
  p->MaybeC |= (tag.MaybeC << sL);
  p->MaybeG |= (tag.MaybeG << sL);
  p->MaybeT |= (tag.MaybeT << sL);
}
static inline void MaybeDegeneratePrimerTag64(MaybeDegeneratePrimer64 *p,MaybeDegeneratePrimer64 tag) {
  if (tag.isD && !(p->isD)) { /* a degenerate tag (is this possible? well if it is, we're ready...) */
    p->p.D = upgradeToDegenerate64(*p); p->isD = 1;
  }
  if(p->isD) DegeneratePrimerTag64(&(p->p.D),upgradeToDegenerate64(tag));
  else PrimerTag64(&(p->p.notD),tag.p.notD);
}
static inline void PrimerTag64B(Primer64 *p,Primer64 tag) {
  /* for backward primers: reverse the tag first, and add it to the lsb of the primer rather than the msb */
  tag = PrimerReverse64(tag);
  int sL = popcount64(tag.valid);
  p->valid = ((p->valid) << sL) | tag.valid;
  p->AorT = ((p->AorT) << sL) | tag.AorT;
  p->GorT = ((p->GorT) << sL) | tag.GorT;
}
static inline void DegeneratePrimerTag64B(DegeneratePrimer64 *p,DegeneratePrimer64 tag) {
  tag = DegeneratePrimerReverse64(tag);
  int sL = popcount64(tag.MaybeA | tag.MaybeC | tag.MaybeG | tag.MaybeT);
  p->MaybeA = ((p->MaybeA) << sL) | tag.MaybeA;
  p->MaybeC = ((p->MaybeC) << sL) | tag.MaybeC;
  p->MaybeG = ((p->MaybeG) << sL) | tag.MaybeG;
  p->MaybeT = ((p->MaybeT) << sL) | tag.MaybeT;
}
static inline void MaybeDegeneratePrimerTag64B(MaybeDegeneratePrimer64 *p,MaybeDegeneratePrimer64 tag) {
  if (tag.isD && !(p->isD)) {
    p->p.D = upgradeToDegenerate64(*p); p->isD = 1;
  }
  if(p->isD) DegeneratePrimerTag64B(&(p->p.D),upgradeToDegenerate64(tag));
  else PrimerTag64B(&(p->p.notD),tag.p.notD);
}
static inline void PrimerRmTag64(Primer64 *p,Primer64 tag) {
  int toRM = popcount64(tag.valid);
  bit64 mask = ~(tag.valid << (popcount64(p->valid) - toRM));
  p->valid &= mask; p->AorT &= mask; p->GorT &= mask;
}
static inline void DegeneratePrimerRmTag64(DegeneratePrimer64 *p,DegeneratePrimer64 tag) {
  bit64 tValid = tag.MaybeA | tag.MaybeC | tag.MaybeG | tag.MaybeT,
    pValid = p->MaybeA | p->MaybeC | p->MaybeG | p->MaybeT;
  int toRM = popcount64(tValid);
  bit64 mask = ~(tValid << (popcount64(pValid) - toRM));
  p->MaybeA &= mask; p->MaybeC &= mask; p->MaybeG &= mask;
  p->MaybeT &= mask;
}
static inline void MaybeDegeneratePrimerRmTag64(MaybeDegeneratePrimer64 *p,MaybeDegeneratePrimer64 tag) {
  if (tag.isD && !(p->isD)) {
    p->p.D = upgradeToDegenerate64(*p); p->isD = 1;
  }
  if(p->isD) DegeneratePrimerRmTag64(&(p->p.D),upgradeToDegenerate64(tag));
  else PrimerRmTag64(&(p->p.notD),tag.p.notD);
}
static inline void PrimerRmTag64B(Primer64 *p,Primer64 tag) {
  int sR = popcount64(tag.valid);
  p->valid >>= sR; p->AorT >>= sR; p->GorT >>= sR;
}
static inline void DegeneratePrimerRmTag64B(DegeneratePrimer64 *p,DegeneratePrimer64 tag) {
  int sR = popcount64(tag.MaybeA | tag.MaybeC | tag.MaybeG | tag.MaybeT);
  p->MaybeA >>= sR; p->MaybeC >>= sR; p->MaybeG >>= sR;
  p->MaybeT >>= sR;
}
static inline void MaybeDegeneratePrimerRmTag64B(MaybeDegeneratePrimer64 *p,MaybeDegeneratePrimer64 tag) {
  if (tag.isD && !(p->isD)) {
    p->p.D = upgradeToDegenerate64(*p); p->isD = 1;
  }
  if(p->isD) DegeneratePrimerRmTag64B(&(p->p.D),upgradeToDegenerate64(tag));
  else PrimerRmTag64B(&(p->p.notD),tag.p.notD);
}

static inline int score64(Primer64 p1,Primer64 p2) {
  /* score the interaction between p1 and p2, fast.
     p2 must have been PrimerReverse64'd by the caller. */
  int sL=(64 - 1/*threshold*/) - leading0_64(p2.valid),
    maxScore = 0; /* this initial value of maxScore is also the minimum score that can be returned.  Do not make it negative without reviewing code that assumes it's >=0 */
  Primer64 p1B; /* we start with p1 shifted left */
  p1B.AorT = p1.AorT << sL; p1B.GorT = p1.GorT << sL;
  p1B.valid = p1.valid << sL;
  int reload = sL - leading0_64(p1.valid);
  if(reload<0) reload=0;
  /* TODO: if rewritten into 2 loops, can reload only once: load in the middle, shift to the right until gone, reload in the middle <<1, while overlap test + shift left.  Of course if the above reload <= 0 then do just the 1 loop as below because it'll be faster in that case. */
  while(1) {
    bit64 overlap = p1B.valid & p2.valid;
    if(!overlap) return maxScore; /* all done */
    bit64 bonds = (~(p1B.AorT ^ p2.AorT)) & (p1B.GorT ^ p2.GorT) & overlap;
    int score = 2*popcount64(bonds) - popcount64(overlap);
    maxScore = (score > maxScore ? score : maxScore);
    if(reload) {
      --sL;
      p1B.AorT = p1.AorT << sL; p1B.GorT = p1.GorT << sL;
      p1B.valid = p1.valid << sL; --reload;
    } else {
      p1B.AorT >>=1; p1B.GorT >>=1; p1B.valid >>=1;
    }
  }
}
static inline int score64D(DegeneratePrimer64 p1,DegeneratePrimer64 p2) {
  bit64 p1Valid = DegenerateValid64(p1),
    p2Valid = DegenerateValid64(p2);
  int sL=(64 - 1/*threshold*/) - leading0_64(p2Valid),
    maxScore = 0;
  DegeneratePrimer64 p1B;
  p1B.MaybeA = p1.MaybeA << sL;
  p1B.MaybeC = p1.MaybeC << sL;
  p1B.MaybeG = p1.MaybeG << sL;
  p1B.MaybeT = p1.MaybeT << sL;
  bit64 p1Bvalid = p1Valid << sL;
  int reload = sL - leading0_64(p1Valid);
  if(reload<0) reload=0;
  while(1) {
    bit64 overlap = p1Bvalid & p2Valid;
    if(!overlap) return maxScore;
    bit64 bonds = (p1B.MaybeA & p2.MaybeT) |
      (p1B.MaybeC & p2.MaybeG) |
      (p1B.MaybeG & p2.MaybeC) |
      (p1B.MaybeT & p2.MaybeA);
    int score = 2*popcount64(bonds) - popcount64(overlap);
    maxScore = (score > maxScore ? score : maxScore);
    if(reload) {
      --sL;
      p1B.MaybeA = p1.MaybeA << sL;
      p1B.MaybeC = p1.MaybeC << sL;
      p1B.MaybeG = p1.MaybeG << sL;
      p1B.MaybeT = p1.MaybeT << sL;
      p1Bvalid = p1Valid << sL; --reload;
    } else {
      p1B.MaybeA >>=1; p1B.MaybeC >>=1; p1B.MaybeG >>=1;
      p1B.MaybeT >>=1; p1Bvalid >>=1;
    }
  }
}
static inline int score64MaybeD(MaybeDegeneratePrimer64 p1,MaybeDegeneratePrimer64 p2) {
  if(p1.isD || p2.isD) return score64D(upgradeToDegenerate64(p1),upgradeToDegenerate64(p2));
  else return score64(p1.p.notD,p2.p.notD);
}

static inline int count64(Primer64 p1,Primer64 p2,
                          int *tried) {
  /* count the number of alignments of >0 bonds,
     for information only.  Similar to score64, but called
     only when outputting interaction data.
     (TODO: could make this return maxScore or minDG as
     well, to save having to call that func separately,
     but low priority because this is called only when
     printing out bonds in excess of threshold) */
  int sL=(64 - 1/*threshold*/) - leading0_64(p2.valid),
    count = 0;
  Primer64 p1B; /* we start with p1 shifted left */
  p1B.AorT = p1.AorT << sL; p1B.GorT = p1.GorT << sL;
  p1B.valid = p1.valid << sL;
  int reload = sL - leading0_64(p1.valid);
  if(reload<0) reload=0;
  while(1) {
    bit64 overlap = p1B.valid & p2.valid;
    if(!overlap) return count;
    (*tried)++;
    if((~(p1B.AorT ^ p2.AorT)) & (p1B.GorT ^ p2.GorT) & overlap) count++;
    if(reload) {
      --sL;
      p1B.AorT = p1.AorT << sL; p1B.GorT = p1.GorT << sL;
      p1B.valid = p1.valid << sL; --reload;
    } else {
      p1B.AorT >>=1; p1B.GorT >>=1; p1B.valid >>=1;
    }
  }
}
static inline int count64D(DegeneratePrimer64 p1,DegeneratePrimer64 p2,int *tried) {
  bit64 p1Valid = DegenerateValid64(p1),
    p2Valid = DegenerateValid64(p2);
  int sL=(64 - 1/*threshold*/) - leading0_64(p2Valid),
    count = 0;
  DegeneratePrimer64 p1B;
  p1B.MaybeA = p1.MaybeA << sL;
  p1B.MaybeC = p1.MaybeC << sL;
  p1B.MaybeG = p1.MaybeG << sL;
  p1B.MaybeT = p1.MaybeT << sL;
  bit64 p1Bvalid = p1Valid << sL;
  int reload = sL - leading0_64(p1Valid);
  if(reload<0) reload=0;
  while(1) {
    bit64 overlap = p1Bvalid & p2Valid;
    if(!overlap) return count;
    (*tried)++;
    if ((p1B.MaybeA & p2.MaybeT) |
        (p1B.MaybeC & p2.MaybeG) |
        (p1B.MaybeG & p2.MaybeC) |
        (p1B.MaybeT & p2.MaybeA)) count++;
    if(reload) {
      --sL;
      p1B.MaybeA = p1.MaybeA << sL;
      p1B.MaybeC = p1.MaybeC << sL;
      p1B.MaybeG = p1.MaybeG << sL;
      p1B.MaybeT = p1.MaybeT << sL;
      p1Bvalid = p1Valid << sL; --reload;
    } else {
      p1B.MaybeA >>=1; p1B.MaybeC >>=1; p1B.MaybeG >>=1;
      p1B.MaybeT >>=1; p1Bvalid >>=1;
    }
  }
}
static inline void count64MaybeD(MaybeDegeneratePrimer64 p1,MaybeDegeneratePrimer64 p2,FILE *f) {
  /* Put clarification for any beginner users who haven't
     been informed that we automatically try all positions
     and print only the worst case */
  int tried = 0;
  int c = ((p1.isD || p2.isD) ? count64D(upgradeToDegenerate64(p1),upgradeToDegenerate64(p2),&tried) : count64(p1.p.notD,p2.p.notD,&tried));
  fprintf(f,"Positions tried: %d\nBonding positions: %d%s\n",tried,c,(c>1)?"  (worst one shown here)":"");
}

static void printBases64(Primer64 p,FILE *f) {
  bit64 i = (bit64)1 << (64-1-leading0_64(p.valid));
  for(; i&p.valid; i>>=1)
    fputc(
         (p.AorT & i) ?
         ((p.GorT & i)?'T':'A') :
         ((p.GorT & i)?'G':'C'), f);
}
static void printBases64D(DegeneratePrimer64 p,FILE *f) {
  bit64 valid = DegenerateValid64(p);
  bit64 i = (bit64)1 << (64-1-leading0_64(valid));
  for(; i&valid; i>>=1) {
    int j = (((p.MaybeA & i)!=0)<<3) |
      (((p.MaybeC & i)!=0)<<2) |
      (((p.MaybeG & i)!=0)<<1) |
      ((p.MaybeT & i)!=0);
    fputc(degenerateCombos[j-1],f);
  }
}
static void printBases64MaybeD(MaybeDegeneratePrimer64 p,FILE *f) {
  if(p.isD) printBases64D(p.p.D,f);
  else printBases64(p.p.notD,f);
}

static void print64_inner(Primer64 p1,int sL,Primer64 p2,bit64 overlap,bit64 bonds,FILE* f) {
  /* code common to print64 and dGprint64 */
  int i1 = leading0_64(p1.valid)-sL, i2 = leading0_64(p2.valid), iMid = leading0_64(overlap); if(i1<i2) { i2-=i1; iMid-=i1; i1=0; } else { i1-=i2; iMid-=i2; i2=0; }
  indent(i1,f); fputs("5'-",f); printBases64(p1,f); fputs("-3'\n",f);
  indent(iMid+(sizeof("5'-")-1), f);
  bit64 bond = (bit64)1 << (64-1-leading0_64(overlap));
  for(; bond&overlap; bond>>=1) fputc((bond&bonds)?'|':'x',f);
  fputc('\n',f); indent(i2,f);
  fputs("3'-",f); printBases64(p2,f);fputs("-5'\n",f);
}
static void print64D_inner(DegeneratePrimer64 p1,int sL,DegeneratePrimer64 p2,bit64 overlap,bit64 bonds,FILE* f) {
      int i1 = leading0_64(DegenerateValid64(p1))-sL, i2 = leading0_64(DegenerateValid64(p2)), iMid = leading0_64(overlap); if(i1<i2) { i2-=i1; iMid-=i1; i1=0; } else { i1-=i2; iMid-=i2; i2=0; }
      indent(i1,f); fputs("5'-",f); printBases64D(p1,f); fputs("-3'\n",f);
      indent(iMid+(sizeof("5'-")-1), f);
      bit64 bond = (bit64)1 << (64-1-leading0_64(overlap));
      for(; bond&overlap; bond>>=1) fputc((bond&bonds)?'|':'x',f);
      fputc('\n',f); indent(i2,f);
      fputs("3'-",f); printBases64D(p2,f); fputs("-5'\n",f);
}
static void print64(Primer64 p1,Primer64 p2,int maxScore,FILE *f) {
  /* maxScore has been found by score64; print a representation
     of the interaction, along with the score */
  int sL=(64 - 1) - leading0_64(p2.valid);
  int sR = 0; Primer64 p1B;
  while(1) {
    if(sL) {
      p1B.AorT = p1.AorT << sL; p1B.GorT = p1.GorT << sL;
      p1B.valid = p1.valid << sL;
    } else {
      /* this function is allowed to be a bit slower than
         score64, and we need to keep all bits */
      p1B.AorT = p1.AorT >> sR; p1B.GorT = p1.GorT >> sR;
      p1B.valid = p1.valid >> sR;
    }
    bit64 overlap = p1B.valid & p2.valid;
    bit64 bonds = (~(p1B.AorT ^ p2.AorT)) & (p1B.GorT ^ p2.GorT) & overlap;
    int score = 2*popcount64(bonds) - popcount64(overlap);
    if(score == maxScore) {
      /* TODO: if more than one ==maxScore, prioritise
         any that has more C-G links, stronger than A-T */
      fprintf(f,"Matches = %d\n",popcount64(bonds));
      fprintf(f,"Score = %d\n",maxScore);
      print64_inner(p1,sL-sR,p2,overlap,bonds,f);
      //return; /* comment out to print ALL maxScore matches */
    }
    if(!overlap) return; /* needed if not returning above */
    if(sL) sL--; else sR++;
  }
}
static void print64D(DegeneratePrimer64 p1,DegeneratePrimer64 p2,int maxScore,FILE *f) {
  int sL=(64 - 1) - leading0_64(DegenerateValid64(p2));
  int sR = 0; DegeneratePrimer64 p1B;
  while(1) {
    if(sL) {
      p1B.MaybeA = p1.MaybeA << sL;
      p1B.MaybeC = p1.MaybeC << sL;
      p1B.MaybeG = p1.MaybeG << sL;
      p1B.MaybeT = p1.MaybeT << sL;
    } else {
      p1B.MaybeA = p1.MaybeA >> sR;
      p1B.MaybeC = p1.MaybeC >> sR;
      p1B.MaybeG = p1.MaybeG >> sR;
      p1B.MaybeT = p1.MaybeT >> sR;
    }
    bit64 overlap = DegenerateValid64(p1B) & DegenerateValid64(p2);
    bit64 bonds = (p1B.MaybeA & p2.MaybeT) |
      (p1B.MaybeC & p2.MaybeG) |
      (p1B.MaybeG & p2.MaybeC) |
      (p1B.MaybeT & p2.MaybeA);
    int score = 2*popcount64(bonds) - popcount64(overlap);
    if(score == maxScore) {
      /* TODO: if more than one ==maxScore, how to
         prioritise the links in the degenerate case? */
      fprintf(f,"Matches = %d\n",popcount64(bonds));
      fprintf(f,"Score = %d\n",maxScore);
      print64D_inner(p1,sL-sR,p2,overlap,bonds,f);
      //return; /* comment out to print ALL maxScore matches */
    }
    if(!overlap) return; /* needed if not returning above */
    if(sL) sL--; else sR++;
  }
}
static void print64MaybeD(MaybeDegeneratePrimer64 p1,MaybeDegeneratePrimer64 p2,const char *name1,const char *name2,int maxScore,FILE *f) {
  if(!name1 || !*name1) name1="(no name)";
  if(!name2 || !*name2) name2="(no name)";
  fprintf(f,"%s versus %s\n",name1,name2);
  count64MaybeD(p1,p2,f);
  if(p1.isD || p2.isD) print64D(upgradeToDegenerate64(p1),upgradeToDegenerate64(p2),maxScore,f);
  else print64(p1.p.notD,p2.p.notD,maxScore,f);
  fputc('\n',f);
}

static void parseFASTA64(char *fileData,MaybeDegeneratePrimer64 *buf,MaybeDegeneratePrimer64 *tags,int *whichTag,char* *names) {
  /* (note: adds extra 0 bytes to fileData) */
  char *seqName=NULL; int p=0;
  int lastByte_to_whichTag[256]; memset(&lastByte_to_whichTag,0xFF,sizeof(lastByte_to_whichTag));
  char check_not_last[256]={0};
  int nextTag = 0;
  fileData += strspn(fileData,"\r\n\xef\xbb\xbf");
  while(*fileData) {
    size_t lineEnd,start=0; do { // multiline seq?
      lineEnd = strcspn(fileData+start,"\r\n\xef\xbb\xbf") + start; /* see comment in load-common.c re stray BOMs */
      start = strspn(fileData+lineEnd,"\r\n\xef\xbb\xbf") + lineEnd;
    } while(*fileData!='>' && fileData[start] && fileData[start]!='>');
    char o=fileData[lineEnd];
    if (*fileData == '>') {
      seqName = fileData+1;
      while(*seqName==' ') seqName++; /* ignore spaces between > and label */
      while(fileData[--lineEnd]==' '); /* and after end of label */
      fileData[++lineEnd]=0;
    } else if (lineEnd) {
      fileData[lineEnd] = 0;
      MaybeDegeneratePrimer64 mdp =
        parseMaybeDegeneratePrimer64(fileData);
      if(!strncmp(seqName,"tag",3) && strlen(seqName)==4){
        unsigned char tagType=fast_toUpper(seqName[3]);
        if (lastByte_to_whichTag[tagType] == -1) {
          /* This tag type hasn't been set before, so
             retroactively apply it to previous primers */
          int p2;
          for(p2=0; p2<p; p2++)
            if(tagType==(unsigned char)fast_toUpper(names[p2][strlen(names[p2])-1]))
              whichTag[p2] = nextTag;
        } else check_not_last[tagType] = 1;
        lastByte_to_whichTag[tagType] = nextTag;
        tags[nextTag++] = mdp;
      } else {
        names[p] = seqName;
        unsigned char tagType=fast_toUpper(seqName[strlen(seqName)-1]);
        whichTag[p] = lastByte_to_whichTag[tagType];
        check_not_last[tagType] = 0;
        buf[p++] = mdp;
      }
    }
    fileData += start; if(!o) break; /* no \n at end ?? */
  }
  p = 0;
  for(nextTag=0; nextTag<256; nextTag++)
    if(check_not_last[nextTag]) {
      if(p) fprintf(stderr,"WARNING: Same applies to >tag%c\n",nextTag);
      else {
        p = 1;
        fprintf(stderr,"\nWARNING: You have multiple >tag%c sequences\n         and the last one does not precede a >...%c primer.\n         This probably means you've made a mistake.\n         Apart from the first >tag%c, all >tag%c tags will apply to\n         >...%c primers AFTER the >tag%c (not before it).\n",nextTag,nextTag,nextTag,nextTag,nextTag,nextTag);
      }
    }
}
static void counts64(const MaybeDegeneratePrimer64 *forward,const MaybeDegeneratePrimer64 *backward,int np,FILE *f) {
  int i,j;
  int counts[64]={0},maxS = 0;
  for(i=0; i<np; i++) for(j=i; j<np; j++) {
      int score = score64MaybeD(forward[i],backward[j]);
      counts[score]++; if(score>maxS) maxS=score;
    }
  for(i=0; i<=maxS; i++) fprintf(f,"%d\t%d\n",i,counts[i]);
  if(f!=stdout) fclose(f);
}
static int pCounts64(const MaybeDegeneratePrimer64 *forward,const MaybeDegeneratePrimer64 *backward,int np,const int *pools,const int *precalcScores) {
  /* like counts64 but includes combinations only if they're in the same pool + count invalid/overlap scores */
  int i,j;
  int counts[64]={0},maxS = 0, other=0;
  for(i=0; i<np; i++) for(j=i; j<np; j++) if(pools[i]==pools[j]) {
        int score = precalcScores ? *precalcScores++ : score64MaybeD(forward[i],backward[j]);
        if(score<64) {
          counts[score]++; if(score>maxS) maxS=score;
        } else other++;
      } else if(precalcScores) precalcScores++;
  int first = 1;
  for(i=0; i<=maxS; i++)
    fprintf(stderr,"%s%d\t%d",first?((first=0),""):"\n",i,counts[i]);
  if(other) fprintf(stderr,"%sOverlaps\t%d",first?"":"\n",other);
  return other;
}
static void printBonds64(const MaybeDegeneratePrimer64 *forward,const MaybeDegeneratePrimer64 *backward,int np,FILE *f,int threshold,char* *names,const int *pools) {
  int i,j;
  ScoreRecord* sr=malloc(t_Nitems(np)*sizeof(ScoreRecord));
  ScoreRecord* sr2 = sr;
  for(i=0; i<np; i++) for(j=i; j<np; j++) {
      if(!pools || pools[i]==pools[j]) {
        int score = score64MaybeD(forward[i],backward[j]);
        if (score >= threshold) {
          if(sr) {
            sr2->score = score; sr2->i = i; (sr2++)->j = j;
          } else print64MaybeD(forward[i],backward[j],names[i],names[j],score,f); /* fallback: print in any order if can't sort by highest score 1st */
        }
      }
    }
  if(sr) {
    qsort(sr,sr2-sr,sizeof(ScoreRecord),highestScore1st);
    ScoreRecord *s;
    for(s=sr; s<sr2; s++)
      print64MaybeD(forward[s->i],backward[s->j],names[s->i],names[s->j],s->score,f);
    free(sr);
  }
}

static int* triangle64(const MaybeDegeneratePrimer64 *forward,const MaybeDegeneratePrimer64 *backward,int np) {
  /* Triangular score cache for pooling purposes */
  int* scores = malloc(t_Nitems(np)*sizeof(int));
  if(scores) {
    int i,j,*p=scores;
    for(i=0; i<np; i++) for(j=i; j<np; j++)
        *p++ = (i==j ? 0 /* ignore self-interaction */ :
                score64MaybeD(forward[i],backward[j]));
  } return scores;
}

static inline bit64 rm_unstable_bonds64(bit64 bonds,bit64 overlap) {
  /* MPprimer_dimer_check.pl's "primer_dimer" function
     does this before its deltaG calculation.  Says
     single matched bases surrounded by mismatches are
     unstable, removes 01 when not followed by valid 1 */
  bit64 lone_1s = bonds & ((~bonds)<<1) & ((~bonds)>>1);
  lone_1s &= (overlap>>1); /* left-hand bit is never got rid of */
  return bonds & ~lone_1s;
}
static inline float deltaG64(Primer64 p1,Primer64 p2,const float* table) {
  /* like score64 but does deltaG instead */
  int sL=(64 - 1/*threshold*/) - leading0_64(p2.valid);
  float minDG = INFINITY;
  Primer64 p1B;
  p1B.AorT = p1.AorT << sL; p1B.GorT = p1.GorT << sL;
  p1B.valid = p1.valid << sL;
  int reload = sL - leading0_64(p1.valid);
  if(reload<0) reload=0;
  while(1) {
    bit64 overlap = p1B.valid & p2.valid;
    if(!overlap) return minDG;
    bit64 bonds = rm_unstable_bonds64((~(p1B.AorT ^ p2.AorT)) & (p1B.GorT ^ p2.GorT) & overlap, overlap);
    int shift = 64-2-leading0_64(bonds); bit64 mask=(bit64)3 << shift,
      maskEnd = (bit64)3 << trail0_64(bonds);
    float dG = table[256+((p1B.AorT & mask)>>(shift+1))]; // init
    for(; mask>=maskEnd; mask>>=1,shift--)
      dG += table[(((p1B.AorT & mask)>>shift)<<6) | (((p1B.GorT & mask)>>shift)<<4) | (((p2.AorT & mask)>>shift)<<2) | ((p2.GorT & mask)>>shift)];
    dG += table[256+((p1B.AorT & mask)>>(shift+1))]; // init at end
    minDG = (dG < minDG ? dG : minDG);
    if(reload) {
      --sL;
      p1B.AorT = p1.AorT << sL; p1B.GorT = p1.GorT << sL;
      p1B.valid = p1.valid << sL; --reload;
    } else {
      p1B.AorT >>=1; p1B.GorT >>=1; p1B.valid >>=1;
    }
  }
}
static inline float deltaG64D(DegeneratePrimer64 p1,DegeneratePrimer64 p2,const float* table) {
  bit64 p1Valid = DegenerateValid64(p1),
    p2Valid = DegenerateValid64(p2);
  int sL=(64 - 1/*threshold*/) - leading0_64(p2Valid);
  float minDG = INFINITY;
  DegeneratePrimer64 p1B;
  p1B.MaybeA = p1.MaybeA << sL;
  p1B.MaybeC = p1.MaybeC << sL;
  p1B.MaybeG = p1.MaybeG << sL;
  p1B.MaybeT = p1.MaybeT << sL;
  bit64 p1Bvalid = p1Valid << sL;
  int reload = sL - leading0_64(p1Valid);
  if(reload<0) reload=0;
  while(1) {
    bit64 overlap = p1Bvalid & p2Valid;
    if(!overlap) return minDG;
    bit64 bonds = rm_unstable_bonds64((p1B.MaybeA & p2.MaybeT) |
                                      (p1B.MaybeC & p2.MaybeG) |
                                      (p1B.MaybeG & p2.MaybeC) |
                                      (p1B.MaybeT & p2.MaybeA),overlap);
    int shift = 64-2-leading0_64(bonds); bit64 mask=(bit64)3 << shift,
      maskEnd = (bit64)3 << trail0_64(bonds);
    float dG = table[256+!(((p1B.MaybeC|p1B.MaybeG) & mask)>>(shift+1))]; // init (worst-case scenario is C or G)
    for(; mask>=maskEnd; mask>>=1,shift--)
      dG += minDGdegenerate((p1B.MaybeA & mask)>>shift,(p1B.MaybeC & mask)>>shift,(p1B.MaybeG & mask)>>shift,(p1B.MaybeT & mask)>>shift,(p2.MaybeA & mask)>>shift,(p2.MaybeC & mask)>>shift,(p2.MaybeG & mask)>>shift,(p2.MaybeT & mask)>>shift,table);
    dG += table[256+!(((p1B.MaybeC|p1B.MaybeG) & mask)>>(shift+1))];
    minDG = (dG < minDG ? dG : minDG);
    if(reload) {
      --sL;
      p1B.MaybeA = p1.MaybeA << sL;
      p1B.MaybeC = p1.MaybeC << sL;
      p1B.MaybeG = p1.MaybeG << sL;
      p1B.MaybeT = p1.MaybeT << sL;
      p1Bvalid = p1Valid << sL; --reload;
    } else {
      p1B.MaybeA >>=1; p1B.MaybeC >>=1; p1B.MaybeG >>=1;
      p1B.MaybeT >>=1; p1Bvalid >>=1;
    }
  }
}
static inline float deltaG64MaybeD(MaybeDegeneratePrimer64 p1,MaybeDegeneratePrimer64 p2,const float* table) {
  if(p1.isD || p2.isD) return deltaG64D(upgradeToDegenerate64(p1),upgradeToDegenerate64(p2),table);
  else return deltaG64(p1.p.notD,p2.p.notD,table);
}
static void dGprint64(Primer64 p1,Primer64 p2,float minDG,FILE *f,const float *table) {
  int sL=(64 - 1) - leading0_64(p2.valid);
  int sR = 0; Primer64 p1B;
  while(1) {
    if(sL) {
      p1B.AorT = p1.AorT << sL; p1B.GorT = p1.GorT << sL;
      p1B.valid = p1.valid << sL;
    } else {
      p1B.AorT = p1.AorT >> sR; p1B.GorT = p1.GorT >> sR;
      p1B.valid = p1.valid >> sR;
      assert(p1B.valid); /* if this breaks, check the range of nearlyEqual */
    }
    bit64 overlap = p1B.valid & p2.valid;
    bit64 bonds0 = (~(p1B.AorT ^ p2.AorT)) & (p1B.GorT ^ p2.GorT) & overlap,
      bonds = rm_unstable_bonds64(bonds0, overlap);
    int shift = 64-2-leading0_64(bonds); bit64 mask=(bit64)3 << shift,
      maskEnd = (bit64)3 << trail0_64(bonds);
    float dG = table[256+((p1B.AorT & mask)>>(shift+1))]; // init
    for(; mask>=maskEnd; mask>>=1,shift--)
      dG += table[(((p1B.AorT & mask)>>shift)<<6) | (((p1B.GorT & mask)>>shift)<<4) | (((p2.AorT & mask)>>shift)<<2) | ((p2.GorT & mask)>>shift)];
    dG += table[256+((p1B.AorT & mask)>>(shift+1))]; // init at end
    if(nearlyEqual(dG,minDG)) {
      fprintf(f,"dG = %.3g\n",dG);
      print64_inner(p1,sL-sR,p2,overlap,bonds0,f);
      return;
    }
    if(sL) sL--; else sR++;
  }
}
static void dGprint64D(DegeneratePrimer64 p1,DegeneratePrimer64 p2,float minDG,FILE *f,const float *table) {
  int sL=(64 - 1) - leading0_64(DegenerateValid64(p2));
  int sR = 0; DegeneratePrimer64 p1B;
  while(1) {
    if(sL) {
      p1B.MaybeA = p1.MaybeA << sL;
      p1B.MaybeC = p1.MaybeC << sL;
      p1B.MaybeG = p1.MaybeG << sL;
      p1B.MaybeT = p1.MaybeT << sL;
    } else {
      p1B.MaybeA = p1.MaybeA >> sR;
      p1B.MaybeC = p1.MaybeC >> sR;
      p1B.MaybeG = p1.MaybeG >> sR;
      p1B.MaybeT = p1.MaybeT >> sR;
      assert(sR < 64); /* if this breaks, check the range of nearlyEqual */
    }
    bit64 overlap = DegenerateValid64(p1B) & DegenerateValid64(p2);
    bit64 bonds0 = (p1B.MaybeA & p2.MaybeT) |
      (p1B.MaybeC & p2.MaybeG) |
      (p1B.MaybeG & p2.MaybeC) |
      (p1B.MaybeT & p2.MaybeA),
      bonds = rm_unstable_bonds64(bonds0,overlap);
    int shift = 64-2-leading0_64(bonds); bit64 mask=(bit64)3 << shift,
      maskEnd = (bit64)3 << trail0_64(bonds);
    float dG = table[256+!(((p1B.MaybeC|p1B.MaybeG) & mask)>>(shift+1))]; // init (worst-case scenario is C or G)
    for(; mask>=maskEnd; mask>>=1,shift--)
      dG += minDGdegenerate((p1B.MaybeA & mask)>>shift,(p1B.MaybeC & mask)>>shift,(p1B.MaybeG & mask)>>shift,(p1B.MaybeT & mask)>>shift,(p2.MaybeA & mask)>>shift,(p2.MaybeC & mask)>>shift,(p2.MaybeG & mask)>>shift,(p2.MaybeT & mask)>>shift,table);
    dG += table[256+!(((p1B.MaybeC|p1B.MaybeG) & mask)>>(shift+1))];
    if(nearlyEqual(dG,minDG)) {
      fprintf(f,"dG = %.3g\n",dG);
      print64D_inner(p1,sL-sR,p2,overlap,bonds0,f);
      return;
    }
    if(sL) sL--; else sR++;
  }
}
static void dGprint64MaybeD(MaybeDegeneratePrimer64 p1,MaybeDegeneratePrimer64 p2,const char *name1,const char *name2,float minDG,FILE *f,const float *table) {
  if(!name1 || !*name1) name1="(no name)";
  if(!name2 || !*name2) name2="(no name)";
  fprintf(f,"%s versus %s\n",name1,name2);
  count64MaybeD(p1,p2,f);
  if(p1.isD || p2.isD) dGprint64D(upgradeToDegenerate64(p1),upgradeToDegenerate64(p2),minDG,f,table);
  else dGprint64(p1.p.notD,p2.p.notD,minDG,f,table);
  fputc('\n',f);
}
static void dGprintBonds64(const MaybeDegeneratePrimer64 *forward,const MaybeDegeneratePrimer64 *backward,int np,FILE *f,float threshold,char* *names,const int *pools,const float *table) {
  DG_ScoreRecord* sr=malloc(t_Nitems(np)*sizeof(DG_ScoreRecord));
  DG_ScoreRecord* sr2 = sr; time_t start=time(NULL);
  time_t next = sr?t_ProgressStart("Sorting... "):0;
  #if defined(_OPENMP)
  #pragma omp parallel
  #endif
  {
  TwoRanges tr=t_iBounds(np);
    int r,i,j,done=0;
    for(r=0; r<2; r++)
    for(i=tr.r[r].start; i<tr.r[r].end; i++,sr?t_Progress("Sorting... ",tr,np,done,&next):0,done+=np-i)
    for(j=i; j<np; j++) {
      if(!pools || pools[i]==pools[j]) {
        float dG = deltaG64MaybeD(forward[i],backward[j],table);
        if (dG <= threshold) {
          #if defined(_OPENMP)
          #pragma omp critical
          #endif
          if(sr) {
            sr2->dG = dG; sr2->i = i; (sr2++)->j = j;
          } else dGprint64MaybeD(forward[i],backward[j],names[i],names[j],dG,f,table);
        }
      }
    }
  }
  if(sr) {
    qsort(sr,sr2-sr,sizeof(DG_ScoreRecord),dGhighestScore1st);
    fputs("\rSorting... done",stderr);
    prnSeconds((long)(time(NULL)-start)); fputs("\n",stderr);
    if(f!=stdout) { fputs("Outputting... ",stderr); start = time(NULL); next = start + 2; }
    fflush(stderr);
    DG_ScoreRecord *s;
    for(s=sr; s<sr2; s++) {
      if(f!=stdout && time(NULL) > next) {
        fprintf(stderr,"\rOutputting... (%d%%) ",100*(int)(s-sr)/(int)(sr2-sr)); fflush(stderr);
        next = time(NULL) + 2;
      }
      dGprint64MaybeD(forward[s->i],backward[s->j],names[s->i],names[s->j],s->dG,f,table);
    }
    free(sr);
    if(f!=stdout) { fputs("\rOutputting... done",stderr); prnSeconds((long)(time(NULL)-start)); fputs("\n",stderr); fflush(stderr); }
  }
}

static int* dGtriangle64(const MaybeDegeneratePrimer64 *forward,const MaybeDegeneratePrimer64 *backward,int np,const float *table) {
  /* To save doing a float version of the pool splitter, emulate 'score' by
     using -dG*2, as bins of size 0.5 should be enough (*10 creates too many empty ones and split_into_pools would need changing) */
  time_t next = t_ProgressStart("Precalculating dG... ");
  time_t start = next;
  int* scores = malloc(t_Nitems(np)*sizeof(int));
  if(scores) 
    #if defined(_OPENMP)
    #pragma omp parallel
    #endif
    {
    TwoRanges tr=t_iBounds(np);
    int r,i,j,*p,done=0;
    for(r=0; r<2; r++)
    for(i=tr.r[r].start, p=scores+t_offset(np,i,i); i<tr.r[r].end; i++,t_Progress("Precalculating dG... ",tr,np,done,&next),done+=np-i)
      for(j=i; j<np; j++)
        *p++ = (i==j ? 0 : dGbucket(deltaG64MaybeD(forward[i],backward[j],table),0x4000-1));
  } fputs("\rPrecalculating dG... done",stderr); prnSeconds((long)(time(NULL)-start)); fputs("\n",stderr); fflush(stderr);
  return scores;
}
static int dGpCounts64(int np,const int *pools,const int *precalcScores,FILE *f) {
  /* this is a combination of counts64 and pCounts64, for the delta-G variant.  pools can be NULL, but not precalcScores */
  int i,j; int*counts=calloc(0x4000+1,sizeof(int)); if(!counts) return 0;
  for(i=0; i<np; i++) for(j=i; j<np; j++) if(!pools || pools[i]==pools[j]) {
        counts[*precalcScores++]++; /* this version has no precalcScores==NULL fallback; could put one in if don't mind passing around the extra parameters */
      } else precalcScores++;
  int lines=0; for(i=0x4000-1; i>0; i--)
                 if(counts[i] && ++lines==20) break;
  int first = 1;
  for(; i<0x4000; i++) if(counts[i]) {
      fprintf(f,"%s%.3g\t%d",first?((first=0),""):"\n",((float)(-i))/2.0,counts[i]);
    }
  int other=counts[0x4000]; free(counts);
  if(other) fprintf(f,"%sOverlaps\t%d",first?"":"\n",other);
  if(f!=stdout && f!=stderr) fclose(f);
  return other;
}
static void dGsCounts64(const MaybeDegeneratePrimer64 *forward,const MaybeDegeneratePrimer64 *backward,int np,const float *table,FILE *f) {
  /* show how much the score can vary around a deltaG range
     (in case anyone thinks it's more accurate than it is) */
  int *counts=calloc(0x4000,sizeof(int)),
    *maxScore=calloc(0x4000,sizeof(int)),
    *minScore=malloc(0x4000*sizeof(int));
  if(memFail(counts,minScore,maxScore,_memFail)) return;
  memset(minScore,0xFF,0x4000*sizeof(int));
  time_t next = t_ProgressStart("Precalculating dG... ");
  time_t start = next;
  #if defined(_OPENMP)
  #pragma omp parallel
  #endif
  {
  TwoRanges tr=t_iBounds(np);
  int r,i,j,done=0;
  for(r=0; r<2; r++)
  for(i=tr.r[r].start; i<tr.r[r].end; i++,t_Progress("Precalculating dG... ",tr,np,done,&next),done+=np-i)
  for(j=i; j<np; j++) {
      float dG = deltaG64MaybeD(forward[i],backward[j],table);
      int score = score64MaybeD(forward[i],backward[j]);
      int bucket = dGbucket(dG,0x4000-1);
      #if defined(_OPENMP)
      #pragma omp critical
      #endif
      {
      counts[bucket]++;
      if(minScore[bucket]<0 || score<minScore[bucket])
        minScore[bucket] = score;
      if(score>maxScore[bucket]) maxScore[bucket] = score;
  }}} fputs("\rPrecalculating dG... done",stderr); prnSeconds((long)(time(NULL)-start)); fputs("\n",stderr); fflush(stderr); int i;
  for(i=0; i<0x4000; i++) if(counts[i]) {
      fprintf(f,"%c%d.%d\t%d\t (score ",i?'-':' ',i/2,(i%2)?5:0,counts[i]);
      if(minScore[i]==maxScore[i]) fprintf(f,"%d)\n",minScore[i]);
      else fprintf(f,"%d-%d)\n",minScore[i],maxScore[i]);
    }
  if(f!=stdout && f!=stderr) fclose(f);
}

static void pStats64(const MaybeDegeneratePrimer64 *forward,const MaybeDegeneratePrimer64 *backward,int np,const int *pools,const int *precalcScores,FILE *f) {
  /* like pCounts64 but outputs per-pool */
  int i,j,nPools=0,pool;
  for(i=0; i<np; i++) if(pools[i]>nPools) nPools=pools[i];
  const int *precalcScores2 = precalcScores; nPools++; /* 1 higher than max */
  for(pool=0; pool<nPools; pool++) {
    fprintf(f,"Pool %d:\n",pool+1);
    precalcScores = precalcScores2;
    int counts[64]={0},maxS = 0, other=0;
    for(i=0; i<np; i++) for(j=i; j<np; j++) if(pools[i]==pools[j] && pools[i]==pool) {
          int score = precalcScores ? *precalcScores++ : score64MaybeD(forward[i],backward[j]);
          if(score<64) {
            counts[score]++; if(score>maxS) maxS=score;
          } else other++;
        } else if(precalcScores) precalcScores++;
    for(i=0; i<=maxS; i++)
      fprintf(f,"%d\t%d\n",i,counts[i]);
    if(other) fprintf(f,"Overlaps\t%d\n",other);
  }
  if(f!=stdout && f!=stderr) fclose(f);
}
static void pStats64dG(int np,const int *pools,const int *precalcScores,FILE *f) {
  int*counts=malloc((0x4000+1)*sizeof(int)); if(memFail(counts,_memFail)) return;
  int i,j,nPools=0,pool;
  for(i=0; i<np; i++) if(pools[i]>nPools) nPools=pools[i];
  const int *precalcScores2 = precalcScores; nPools++;
  for(pool=0; pool<nPools; pool++) {
    fprintf(f,"Pool %d:\n",pool+1);
    memset(counts,0,(0x4000+1)*sizeof(int));
    precalcScores = precalcScores2;
  for(i=0; i<np; i++) for(j=i; j<np; j++) if(pools[i]==pools[j] && pools[i]==pool) {
        counts[*precalcScores++]++;
      } else precalcScores++;
  int lines=0; for(i=0x4000-1; i>0; i--)
                 if(counts[i] && ++lines==20) break;
  for(; i<0x4000; i++) if(counts[i]) {
      fprintf(f,"%.3g\t%d\n",((float)(-i))/2.0,counts[i]);
    }
  int other=counts[0x4000];
  if(other) fprintf(f,"Overlaps\t%d\n",other);
  } free(counts);
  if(f!=stdout && f!=stderr) fclose(f);
}
