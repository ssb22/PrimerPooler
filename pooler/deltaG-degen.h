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
#include <math.h> /* for fabs */
/* DeltaG calculations for degenerate primers.  Worst-case is lowest deltaG.
   8-bit table offsets: AorTF1,AorTF2,GorTF1,GorTF2,AorTR1,AorTR2,GorTR1,GorTR2
   We could precompute a 64k lookup table for all MaybeACGT combinations, but
   will have to be done separately for each temperature etc, so if degenerate
   primers are rare then might as well do this:
 */
static inline float minimise(float a,float b) { return a<b ? a : b; }
static float _minDGdegenerate(int valSoFar,int shift,const int *inBits,const float *table) {
  /* AorT 0 GorT */ enum { A=4,C=0,G=1,T=5 }; /* shift 5,4,1,0 */
  if(shift==3) shift=1; else if(shift<0) return table[valSoFar]; float m=0;
  if(inBits[0]) m=minimise(m,_minDGdegenerate(valSoFar|(A<<shift),shift-1,inBits+4,table));
  if(inBits[1]) m=minimise(m,_minDGdegenerate(valSoFar|(C<<shift),shift-1,inBits+4,table));
  if(inBits[2]) m=minimise(m,_minDGdegenerate(valSoFar|(G<<shift),shift-1,inBits+4,table));
  if(inBits[3]) m=minimise(m,_minDGdegenerate(valSoFar|(T<<shift),shift-1,inBits+4,table));
  return m;
}
static inline float minDGdegenerate(int MaybeA12,int MaybeC12,int MaybeG12,int MaybeT12,int MaybeA34,int MaybeC34,int MaybeG34,int MaybeT34,const float *table) {
  int inBits[16]; // [a,c,g,t, a,c,g,t, a,c,g,t, a,c,g,t]
  inBits[0] = MaybeA12 >> 1; inBits[1] = MaybeC12 >> 1;
  inBits[2] = MaybeG12 >> 1; inBits[3] = MaybeT12 >> 1;
  inBits[4] = MaybeA12 & 1; inBits[5] = MaybeC12 & 1;
  inBits[6] = MaybeG12 & 1; inBits[7] = MaybeT12 & 1;
  inBits[8] = MaybeA34 >> 1; inBits[9] = MaybeC34 >> 1;
  inBits[10] = MaybeG34 >> 1; inBits[11] = MaybeT34 >> 1;
  inBits[12] = MaybeA34 & 1; inBits[13] = MaybeC34 & 1;
  inBits[14] = MaybeG34 & 1; inBits[15] = MaybeT34 & 1;
  return _minDGdegenerate(0,5,inBits,table);
}

static inline int nearlyEqual(float a,float b) {
  /* After having calculated minDG, the output code
     ought to be able to say if dG==minDG to check
     it's seeing the minimum one again.  In theory,
     rounding error shouldn't matter as long as we
     take the EXACT same steps for both calculations
     (rouding errors should be exactly identical).
     In practice, when compiling with optimisation,
     some of GCC's optimisations result in floating-
     point calculations becoming slightly different
     in different places (reordering etc), and it's
     NOT guaranteed that you'll get the exact same
     bits from two different parts of the program
     that seemingly do the same thing!  So we have
     to do the "nearly equal" thing.
     
     TODO: this won't work at threshold 0 if there
     are very small <0 deltaG's (but those are not
     likely to be accurate anyway: take them out?
     insist on a threshold of at least -1 ?) could
     adapt the 0.001 but beware of slowing it down
     (if b is a min, try b += fabs(b)*0.0001 before
     the loop)
  */
  return fabs(a-b) <= 0.001;
}
