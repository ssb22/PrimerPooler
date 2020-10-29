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
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include "numbers.h"
#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>
#include <direct.h>
#define getcwd _getcwd
#endif
#include "ansi.h"
#include "all-primers.h"
#include "deltaG.h"
#include "memcheck.h"
static char *argv0; static int numQu = 0;
void getAns(const char *qu, char *buf,size_t size) {
  numQu++;
  while(1) {
    fflush(stdout); /* see below */
    SetBold(); fputs(qu,stderr);
    fflush(stderr); /* shouldn't be necessary, but we all
                       know how buggy Windows is....
                       (some versions of WINE need this at
                       least; don't know about real Win)*/
    ResetColour();
    #ifdef __linux__
    static int workedBefore = 0;
    #endif
    if(!fgets(buf,size,stdin)) {
#ifdef __linux__
      if(!workedBefore) {
        /* Probably we were launched by a graphical
           desktop that didn't give us a terminal.
           
           (Windows and Mac automatically start a terminal
           if we're launched via the graphical desktop,
           but not all GNU/Linux graphical desktops do so)
        */
        puts("Early EOF on stdin: assuming a launcher problem.\nTrying to run argv[0] in a terminal.");
        /* 1. Try to run in lxterminal.  Care is needed
           if any part of argv0 contains a space etc, as
           lxterminal will split on space when we don't
           want it to.  This workaround uses multiple
           levels of quoting, unless argv0 is simple so as
           not to require it. */
        static const char *okChars="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_-./=";
        if(!argv0[strspn(argv0,okChars)])
          execl("/usr/bin/lxterminal","/usr/bin/lxterminal","-e",argv0,NULL);
        else {
          char *buf=malloc(strlen(argv0)*4+12);
          if(buf) {
            memcpy(buf,"'$'\"'\"'",6);
            int i,w=7; for(i=0; argv0[i]; i++) {
              if(strchr(okChars,argv0[i]))
                buf[w++]=argv0[i];
              else w += sprintf(buf+w,"\\x%02x",argv0[i]);
            }
            strcpy(buf+w,"'\"'\"");
            execl("/usr/bin/lxterminal",
                  "/usr/bin/lxterminal","-e",
                  "/bin/bash","-c",buf,NULL);
            free(buf);
          }
        }
        /* 2. Try gnome-terminal.  This doesn't need
           quoting, as long as we use its -x not its -e */
        execl("/usr/bin/gnome-terminal",
              "/usr/bin/gnome-terminal","-x",
              argv0,NULL);
        /* 3. Try rxvt and xterm.  These programs don't
           need us to do the above complex quoting.
           (Weren't put first because lxterminal and
           gnome-terminal is more likely to be configured
           with the user's preferred fonts etc.)
        */
        execl("/usr/bin/rxvt","/usr/bin/rxvt","-e",
              argv0,NULL);
        execl("/usr/bin/xterm","/usr/bin/xterm","-e",
              argv0,NULL);
        puts("... failed; bailing out.");
      }
#endif
      exit(0);
    }
#ifdef __linux__
    workedBefore = 1;
#endif
    size_t l = strcspn(buf,"\r\n");
    if(!buf[l]) {
      puts("Do you really expect me to process an answer THAT long?");
      while(!buf[l]) {
        if(!fgets(buf,size,stdin)) exit(0);
        l = strcspn(buf,"\r\n");
      } continue;
    }
    buf[l]=0;
    if(*buf) { /* just check it's all ASCII */
      char *p;
      for(p=buf; !(*p&0x80); p++)
        if(!*p) return;
      puts("ASCII-only answers please (check your input method)"); /* and repeat the qu */
    } /* else blank line: repeat the qu */
  }
}

static int getYN(const char *qu) {
  char buf[80];
  while(1) {
    getAns(qu,buf,sizeof(buf));
    switch(*buf) {
    case 'y': case 'Y':
      putchar('\n');
      return 1;
    case 'n': case 'N':
      putchar('\n');
      return 0;
    default: puts("That wasn't a Y or N answer.");
    }
  }
}

static int getNum(const char *qu,int min) {
  char buf[80]; /* long enough for the non-digit 'problem' inputs we want to identify more specifically than 'too long' */
  while(1) {
    getAns(qu,buf,sizeof(buf));
    char *endPtr; long n = strtol(buf,&endPtr,10);
    int valid = 1;
    if(n>=1000000) { /* might have overflowed */
      printf("Ha ha ... how big is YOUR grant?\n");
      valid = 0;
    } if(*endPtr) { /* trailing input */
      printf("\"%s\" doesn't work:\n  please enter a whole number in digits.\n",endPtr);
      valid = 0; if(!n) continue; /* don't say "nought" */
    } if (n<0) {
      printf("A minus number?  Are you trying to break me?\n"); valid = 0;
    } else if(n==0) {
      printf("Nought?  Are you trying to be funny?\n");
      valid = 0;
    }
    if(valid) {
      if(n<min) { printf("Cannot possibly solve for lower than %d/pool!\n",min); continue; } /* for maxCount */
      putchar('\n');
      switch(0){case 0:case sizeof(int)>=4:;} /* if this fails, your int size is too small */
      return(int)n;
    }
  }
}

static float getFloat(const char *qu) {
  char buf[80];
  while(1) {
    getAns(qu,buf,sizeof(buf));
    char *endPtr; float n = strtof(buf,&endPtr);
    int valid = 1;
    if(n>1000000 || n<-1000000) { /* might have overflowed */
      printf("Ha ha ... how big is YOUR grant?\n");
      valid = 0;
    } if(*endPtr) { /* trailing input */
      printf("\"%s\" doesn't work:\n  please enter a number in digits.\n",endPtr);
      valid = 0;
    }
    if(valid) {
      putchar('\n'); return n;
    }
  }
}

static FILE* getFile_graphical(const char *qu,const char *mode) {
#if defined(_WIN64) || defined(_WIN32)
  const char *typeIt = "OK, please type the name here e.g. XYZ.txt or C:\\Documents\\XYZ.txt";
  if(getYN("Do you want to find the file in Windows instead of typing it here? (y/n): ")) {
    while(1) {
      puts("OK, I'm starting the Windows file finder.\nIf you don't see it, check BEHIND this window\n(some versions of Windows don't put it in front; blame Microsoft not me)");
      OPENFILENAME of={0}; of.lStructSize = sizeof(of);
      char buf[1024]={0};
      of.lpstrFile = buf;
      of.nMaxFile = sizeof(buf);
      of.lpstrTitle = qu;
      if(GetOpenFileName(&of)) {
        FILE *f = fopen(buf,mode);
        if(f) {
          printf("\nSuccessfully opened %s\n",buf);
          return f;
        }
        if (getYN("Something went wrong opening that file.  Try again? (y/n): ")) continue;
      } else if (getYN("The Windows file finder was cancelled. Try it again? (y/n): ")) continue;
      puts(typeIt); break;
    }
  } else puts(typeIt);
#endif
#ifdef __linux__
  FILE *p = popen("which zenity 2>/dev/null","r"); // GNOME 2
  if(!p) return NULL;
  char buf[1024];
  char *b = fgets(buf,sizeof(buf),p);
  pclose(p); if(!b || !*buf) return NULL;
  buf[strlen(buf)-1]=0;
  p = fopen(buf,"r"); if(!p) return NULL; // output of 'which' does not exist
  fclose(p);
  if(getYN("Do you want to find the file with a chooser instead of typing it here? (y/n): ")) {
    p = popen("zenity --file-selection","r");
    if(!p) puts("Sorry, zenity won't run.");
    else {
      if(!fgets(buf,sizeof(buf),p)) { puts("Problem reading answer from zenity."); *buf = 0; }
      if(*buf) buf[strlen(buf)-1]=0;
      pclose(p); p = fopen(buf,mode);
      if(p) {
        printf("\nSuccessfully opened %s\n",buf);
        return p;
      }
    }
  }
#endif
#ifdef __APPLE__
  FILE *p=fopen("/usr/bin/osascript","r");
  if(!p) return NULL;
  if(getYN("Do you want to find the file in Mac Finder instead of typing it here? (y/n): ")) {
    p = popen("osascript -e $'tell application \"System Events\"\\nactivate\\nset f to choose file\\nend tell\\nPOSIX path of f'","r");
    if(!p) return NULL;
    char buf[1024], *b=fgets(buf,sizeof(buf),p);
    pclose(p); if(!b || !*buf) return NULL;
    buf[strlen(buf)-1]=0;
    p = fopen(buf,mode);
    if(p) {
      printf("\nSuccessfully opened %s\n",buf);
      return p;
    }
  }
#endif
  return NULL;
}

static const char DirSep =
#if defined(_WIN64) || defined(_WIN32)
    '\\'
#else
    '/'
#endif
  ;
static void prnCWD() {
  char buf[1024]; /* TODO: PATH_MAX or similar? */
  if(getcwd(buf,sizeof(buf))) printf("Current directory (folder) is: %s%c\n",buf,DirSep);
}

static FILE *getFile(const char *qu,const char *mode,const char *default_fname,const char *url) {
  if(*mode=='r') { /* TODO: folder select and default_fname for 'w' ?? */
    if(default_fname) {
      FILE *f=fopen(default_fname,mode);
      if(f) {
        printf("%s\nI found %s in this folder.\n",qu,default_fname);
        if(getYN("Shall I use that? (y/n): ")) return f;
        fclose(f); puts("OK.");
      }
    }
    if(url) {
      printf("You might be able to find an appropriate file at\n%s\n",url);
#if defined(_WIN64) || defined(_WIN32) || defined(__APPLE__) || defined(__linux__)
      if (getYN("Would you like me to point your browser there? (y/n): ")) {
        char buf[200];
        snprintf(buf,sizeof(buf),"%s %s",
#if defined(_WIN64) || defined(_WIN32)
                 "start \"%ProgramFiles%\\Internet Explorer\\iexplore.exe\""
#endif
#ifdef __APPLE__
                 "open"
#endif
#ifdef __linux__
                 "xdg-open"
#endif
                 ,url);
        int ret=system(buf);
        if(ret) puts("Problem opening the browser!");
        else if(default_fname) printf("I've tried to open a browser so you can download a file like %s\n",default_fname);
        printf("If the browser doesn't open, please open one yourself and go to\n%s\nOnce downloaded, please tell me where you put it.\n",url);
      } else puts("OK, please give me another file to use.");
#endif
    }
    FILE *f = getFile_graphical(qu,mode);
    if(f) return f;
  }
  prnCWD();
  char buf[1024]; /* TODO: PATH_MAX or similar? */
  while(1) {
    getAns(qu,buf,sizeof(buf));
    if(*mode=='w' && !strchr(buf,'.') && getYN("You didn't specify an extension.  Shall I add \".txt\"? (y/n): ")) strncat(buf,".txt",(sizeof(buf)-1)-strlen(buf));
    if(*mode=='w') {
      FILE *t = fopen(buf,"r");
      if(t) { fclose(t); if(!getYN("That file already exists.  Are you sure you want to overwrite it? (y/n): ")) continue; }
    }
    FILE *f = fopen(buf,mode);
    if(f) return f;
    puts("I could not open that file.  Are we in the right folder?");
  }
}

static FILE *resultsFileInner() {
  return getFile("Name of new results file: ","w",NULL,NULL); /* TODO: "wb" and ensure all platforms write \r\n so resulting files can be copied to Windows and opened in Notepad even when we're not running on Windows? */
}

static FILE *getResultsFile() {
  return getYN("Do you want to see the results on the screen now?\n(Answer 'no' if you want to save them to a file instead.)\nShow on screen? (y/n): ")?stdout:resultsFileInner();
}

static FILE *getResultsFile2(FILE *f) {
  return (f==stdout && getYN("Do you want to save this to a file as well? (y/n): ")) ? resultsFileInner() : NULL;
}

static FILE *open_or_exit(const char *filename,const char*mode) {
  /* for command-line use */
  if(!strcmp(filename,"-")) return (mode[0]=='r') ? stdin : stdout; /* new in v1.2 */
  FILE *f = fopen(filename,mode);
  if(!f) {
    fprintf(stderr,"Unable to open %s\n",filename);
    exit(1);
  }
  return f;
}

static void pools2files(const char *prefix,const int *pools,int nPools,AllPrimers ap) {
  int i; char buf[100];
  for(i=0; i<nPools; i++) {
    snprintf(buf,sizeof(buf),"%s%d.txt",prefix,i+1);
    FILE *f=fopen(buf,"w");
    if(f) {
      printFASTA(ap,f,pools,i); fclose(f);
      fprintf(stderr,"Wrote %s\n",buf);
    } else fprintf(stderr,"Could not write to %s\n",buf);
  }
}

static void allPoolsToOneFile(FILE *f,int *pools,int nPools,AllPrimers ap) {
  int i; for(i=0; i<nPools; i++) {
    fprintf(f,">\n>\n> --- Pool %d of %d ---\n>\n>\n",i+1,nPools); printFASTA(ap,f,pools,i);
  }
  if(f!=stdout) fclose(f);
}

static float getKelvin0(const char *qu) {
  while(1) {
    float r = getFloat(qu);
    int tried_R=0, tried_K=0, tried_F=0;
    if (r > 400) {
      tried_R = 1;
      if (getYN("What?  Are you using the Rankine scale? (y/n): ")) return R_to_kelvin(r);
    }
    if (r > 270) {
      tried_K = 1;
      if (getYN(tried_R?"Seriously, kelvin? (y/n): ":"Is that Kelvin? (y/n): ")) return r;
    }
    if (r > 90) {
      tried_F = 1;
      if (getYN("Hmmm... Fahrenheit? (y/n): ")) return F_to_kelvin(r);
    }
    if(getYN(tried_F?"Seriously, Celsius ? (y/n): ":"Is that C ? (y/n): ")) return C_to_kelvin(r);
    if (!tried_F && getYN("Seriously, Fahrenheit? (y/n): ")) return F_to_kelvin(r);
    if (!tried_K && getYN("Kelvin?? (y/n): ")) return r;
    if (!tried_R && getYN("Rankine?? (y/n): ")) return R_to_kelvin(r);
    if(getYN("Reaumur?? (y/n): ")) return C_to_kelvin(r*1.25);
    if(getYN("Romer?? (y/n): ")) return C_to_kelvin((r-7.5)*40/21);
    if(getYN("Delisle?? (y/n): ")) return C_to_kelvin(100-r*2/3);
    if(getYN("Average translational kinetic energy of a gas in zeptojoules?? (y/n): ")) return r*2/3/.01380649;
    if(getYN("Randall Munroe's joke 'Felsius' unit??? (y/n): ")) return C_to_kelvin((r-16.0)*5.0/7.0);
    puts("I give up.  You'll have to pick a unit that I know.");
  }
}
static float getKelvin(const char *qu) {
  while (1) {
    float k = getKelvin0(qu);
    if (k < 0) {
      if (!getYN("Do you have a way to defy the laws of physics? (y/n): ")) continue;
      puts("OK superhero.  Try not to destroy the universe while you're at it.");
    }
    else if (k <= 274 && !getYN("That's COLD.  Do you have high-pressure cryonics or something? (y/n): ")) continue;
    else if (k >= 373 && !getYN("That's HOT.  Do you seriously have a way to do this? (y/n): ")) continue;
    return k;
  }
}

static float* getDeltaG(int maxLen) {
  puts("To sort by deltaG, I'll need temperature and concentration settings.\nOr we can skip it and use score (faster and simpler but less accurate).");
  float *table = NULL;
  if(getYN("Do you want to use deltaG? (y/n): ") || (maxLen >= 35 && !getYN("Your primers (including any tags) are seriously long.\nScore is even LESS accurate when using long primers and/or tags.\nAre you REALLY SURE you do NOT want to use deltaG? (y/n): "))) {
    puts("OK, please enter the deltaG settings:");
    float k,mag,mono,dntp;
    while (1) {
      k=getKelvin("Temperature: ");
      if (k >= 0 || getYN("This temperature is physically impossible. Continue anyway? (y/n): ")) break;
    }
    while(1) {
      mag=getFloat("Magnesium concentration in mM (0 for no correction): "); // Versions prior to 1.17 incorrectly called these nM instead of mM (unit, you nit :-) )
      mono=getFloat("Monovalent cation (e.g. sodium) concentration in mM (e.g. 50): ");
      dntp=getFloat("dNTP concentration in mM (0 for no correction): ");
      if (mag || mono || dntp || getYN("Zeroing all 3 concentrations will give infinite deltaG.\nCarry on regardless? (y/n): ")) break;
    }
    table = deltaG_table(k,mag,mono,dntp);
    if(!table) puts("Out of memory: falling back to score (no deltaG)");
  } return table;
}
static void printBondsLoop(AllPrimers ap,const int *pools,float *table) {
  do {
    int threshold = 0; float dG_threshold = 0;
    if(table) {
      dG_threshold = getFloat("What dG threshold shall we go for? ");
      if (dG_threshold > 0 && getYN("You entered a positive number.  Shall I negate it? (y/n): ")) dG_threshold = -dG_threshold;
    } else threshold = getNum("What score threshold shall we go for? ",0);
    FILE *f=getResultsFile();
    addTags(ap);
    if(table) dGprintBonds(ap,f,dG_threshold,pools,table);
    else printBonds(ap,f,threshold,pools);
    f = getResultsFile2(f);
    if(f) {
      if(table) dGprintBonds(ap,f,dG_threshold,pools,table);
      else printBonds(ap,f,threshold,pools); }
    removeTags(ap);
  } while(getYN("Do you want to try another threshold? (y/n): "));
}

static char *arg0() {
  char *c=strrchr(argv0,DirSep);
  if(c) return c+1;
  return argv0;
}

static void assertHandler(int s) {
  /* in case we're running in interactive mode and the
     console window disappears on abort() */
  signal(SIGABRT, SIG_DFL);
  puts("THIS SHOULD NEVER HAPPEN.\nPress Enter to quit."); getchar(); }

static int anotherGo() {
  if(numQu>=20) printf("You have answered %d questions.  Well done :-)\n",numQu);
  numQu = 0;
  return getYN("Would you like another go? (y/n): ");
}

static int averagePairsRoundUp(int np,int nPools) {
  np = (np+1)/2; return (np+nPools-1)/nPools;
}

static void suggestMax(int nPools,int average,int total) {
  printf("With these parameters, we must allow at least %d per pool, so:\n",average);
  int powerOfTen = (average<=1000) ? 10 : 100;
  int suggestion = (average/powerOfTen+1)*powerOfTen;
  int numSuggestionsOutput;
  for(numSuggestionsOutput=0;numSuggestionsOutput<4;
      suggestion+=powerOfTen) {
    int remaining=total, i, min = suggestion;
    for(i=0; i<nPools; i++)
      if(remaining < suggestion) {
        /* worst case: */
        min = (nPools > i+1) ? 0 : remaining;
      break;
      } else remaining -= suggestion;
    int difference = suggestion-min;
    int percent = 100*difference/suggestion;
    if(percent>30 && numSuggestionsOutput) break;
    if(percent==100) {
      printf("I suggest max difference %d for room to manoeuvre\n(I suggest don't worry about percentages with these small pools)\n",suggestion);
      break;
    } else if(percent>10) {
      printf("For max difference %d (%d%%), set max/pool=%d\n",difference,percent,suggestion);
      numSuggestionsOutput++; }
  }
}

int main(int argc, char *argv[]) {
  argv0 = argv[0]; InitNumbers();
  void printTitle(); printTitle(); // title.c
  if(argc==1) {
    puts("You did not specify any command-line arguments.");
    if(getYN("Would you like to run interactively? (y/n): ")) {
      signal(SIGABRT, assertHandler);
      do {
        puts("Please enter the name of the primers file to read.\n"
             "I'm expecting a text file in multiple-sequence FASTA format;\n"
             "it is allowed to use degenerate bases.\n"
             "Names of amplicons' primers should end with F or R, and otherwise match.\n"
             "(Optionally include Taq probes etc ending with P/Q/etc.\n"
             "All names differing in only the last letter will be kept in the same pool.\n"
             "To force a pool choice, put @2: at start of primer name for pool 2.)\n"
             "Optionally include tags to apply to all primers: >tagF and >tagR\n"
             "(you can change these mid-file if you want to vary your tags)"); // tags are applied based on their last letter (tagN)
        AllPrimers ap=loadFASTA(getFile("File name: ","rb",NULL,NULL));
        if(ap.np>0) {
          float *table = getDeltaG(ap.maxLen);
          if(getYN(table ? "Shall I count how many pairs have what deltaG range? (y/n): " : "Shall I count how many pairs have what score? (y/n): ")) {
            puts("OK, just a sec..."); fflush(stdout);
            addTags(ap);
            if(table) dGandScoreCounts(ap,table,stdout);
            else printCounts(ap,stdout);
            putchar('\n');
            if(getYN("Do you want to save this to a file? (y/n): ")) {
              FILE *f=getFile("File name: ","w",NULL,NULL);
              if(table) dGandScoreCounts(ap,table,f);
              else printCounts(ap,f);
            }
            removeTags(ap);
          }
          if(getYN("Do you want to see the highest bonds of the whole file? (y/n): ")) printBondsLoop(ap,NULL,table);
          if(getYN("Shall I split this into pools for you? (y/n): ")) {
            int nAmplicons=0; char *overlappingAmplicons=NULL; int *primerNoToAmpliconNo=NULL;
            if(getYN("Shall I check the amplicons for overlaps in the genome? (y/n): ")) {
              puts("OK, I need to see a genome file in .2bit or .fa format.");
              FILE *f=getFile("File name: ","rb","hg38.2bit","http://hgdownload.cse.ucsc.edu/downloads.html"); // if human-specific, http://hgdownload.soe.ucsc.edu/goldenPath/hg38/bigZips/
              int ignoreVars = getYN("Do you want me to ignore variant chromosomes\ni.e. sequences with _ or - in their names? (y/n): ");
              puts("Please enter the maximum amplicon length (bp), for example 220\n"); // bp = base pairs
              int ampLen = getNum("Maximum amplicon length: ",0);
              int multiplx = getYN("Do you want to write a file of amplicons for MultiPLX as well? (y/n): ");
              FILE *allAmps = (multiplx || getYN("or do you want to write a file with the locations of all amplicons? (y/n): "))?getFile("File name for all amplicon locations: ","w",NULL,NULL):NULL;
              overlappingAmplicons=GetOverlappingAmplicons(ap,f,&primerNoToAmpliconNo,&nAmplicons,ampLen,allAmps,multiplx,ignoreVars);
            }
            PS_cache cache=PS_precalc(ap,table,overlappingAmplicons,primerNoToAmpliconNo,nAmplicons);
            int seedless = -1;
            printf("Preliminary pooling by threshold...\n");
            printf("Computer suggestion is %d pools.\nYou can go with this, or you can pick your own number.\n",suggest_num_pools(ap,cache,table));
            do {
              int nPools = getNum("How many pools? ",0);
              if(nPools<=1) { puts("Cannot divide into fewer than 2 pools."); continue; }
              else if(nPools<cache.fix_min_pools) { printf("Must be at least %d pools, because you have primers with names starting @%d:\n",cache.fix_min_pools,cache.fix_min_pools); continue; }
              int average = 2*averagePairsRoundUp(ap.np,nPools); /* important to round UP, for the getNum below */
              puts("Setting a maximum size of each pool can make the pools more even.");
              suggestMax(nPools,average,ap.np);
              int maxCount = 0;
              if (getYN("Do you want to set a maximum? (y/n): "))
                while(1) {
                  maxCount = getNum("Maximum size of each pool: ",average);
                  if (maxCount > average || getYN("Setting the maximum count exactly equal to the average is a BAD idea.\nIt will severely restrict the program's movement of primers between pools,\nmeaning pools cannot be optimised and the only progress will\nbe by repeated re-randomization from the beginning.\nAre you sure you're happy with this? (y/n): ")) break;
                  suggestMax(nPools,average,ap.np);
                }
              int limit = getYN("Do you want to give me a time limit? (y/n): ")?getNum("How many minutes? ",0):0;
              int *pools = split_into_pools(ap,nPools,limit,cache,(seedless==-1)?(seedless=getYN("Do you want my \"random\" choices to be 100% reproducible for demonstrations? (y/n): "))!=0:seedless,table,maxCount);
              if(pools) {
                if(getYN("Do you want to see the statistics of each pool? (y/n): ")) {
                  FILE *f=getResultsFile();
                  if(table) dGprintStats(ap,pools,cache.scores,f); else printStats(ap,pools,cache.scores,f);
                  f = getResultsFile2(f);
                  if(f) {
                    if(table) dGprintStats(ap,pools,cache.scores,f); else printStats(ap,pools,cache.scores,f); }}
                if(getYN("Do you want to see the highest bonds of these pools? (y/n): ")) printBondsLoop(ap,pools,table);
                if(getYN("Shall I write each pool to a different result file? (y/n): ")) {
                  puts("OK, let's call them file1.txt, file2.txt etc.\nBut what prefix shall I use instead of 'file'?");
                  prnCWD(); char buf[80];
                  getAns("Prefix: ",buf,sizeof(buf));
                  pools2files(buf,pools,nPools,ap);
                } else if(getYN("OK, shall I merge all pools and put a banner above each? (y/n): ")) {
                  FILE *f=getResultsFile();
                  allPoolsToOneFile(f,pools,nPools,ap);
                  f = getResultsFile2(f);
                  if(f) allPoolsToOneFile(f,pools,nPools,ap);
                } else puts("OK, I'll forget about those pools.");
                free(pools);
              }
            } while(getYN("Do you want to try a different number of pools? (y/n): "));
            PS_free(cache);
            if(overlappingAmplicons) free(overlappingAmplicons);
            if(primerNoToAmpliconNo) free(primerNoToAmpliconNo);
          } puts("OK, I'll forget about those primers."); freeAllPrimers(ap);
          if(table) free(table);
        }
      } while(anotherGo()); puts("Bye.\n");
    } else {
      printf("OK, bye for now.\nRun %s --help to show command-line help.\n\n",arg0());
      exit(0);
    }
  } else { // command line
    int doCounts = 0, numPools = 0,
      mins = 0, hasThreshold = 0, selfOmit = 0;
    float threshold = 0, *table = NULL;
    FILE *genomeFile = NULL, *multiplxFile = NULL;
    char *poolPrefix=NULL;
    AllPrimers ap={0,0,0,0,0,0,0};
    int maxAmpliconLen = 220; // also in help text
    int seedless = 0, maxCount = 0, suggestPools = 0;
    int ignoreVars = 1; // (default 1 for drop-in compatibility with v1.61 and below, which didn't have the option to turn it off)
    int i; for(i=1; i<argc; i++) {
      if(!strcmp(argv[i],"--help") || !strcmp(argv[i],"/help") || !strcmp(argv[i],"/?")) {
        printf("%s [options] FASTA-file\n",arg0());
        puts("Options can include any of:");
        puts("--counts  (show score or deltaG-range pair counts for whole input)");
        puts("--self-omit (omits self-interaction and pair-interaction in --counts)");
        puts("--print-bonds=THRESHOLD e.g. --print-bonds=1");
        puts("--dg[=KELVIN[,mg[,cation[,dNTP]]]] to use dG instead of score");
        puts("--suggest-pools suggests a number for --pools (or use --pools=?)");
        puts("--pools[=NUM[,MINS[,PREFIX]]] e.g. --pools=2,1,poolfile-");
        puts("(Set prefix to a single hyphen (-) to write all to stdout)");
        puts("--max-count=NUM (per pool)");
        puts("--genome=PATH to check amplicons for overlaps in the genome (.2bit or .fa)");
        puts("--scan-variants scans variant sequences in the genome too (_ and - in names)");
        puts("--amp-max=LENGTH sets max amplicon length for the overlap check (default 220)"); /* v1.35 added 0 = unlimited, not available in interactive version */
        puts("--multiplx=FILE (write MultiPLX input after the --genome stage)");
        puts("--seedless (don't seed random number generator)");
        exit(0);
      } else if(!strcmp(argv[i],"--version")) {
        /* TODO: document this option under --help ? */
        exit(0);
      } else if(!strcmp(argv[i],"--counts"))
        doCounts = 1;
      else if(!strcmp(argv[i],"--self-omit"))
        selfOmit = 1;
      else if(!strncmp(argv[i],"--print-bonds=",sizeof("--print-bonds=")-1)) {
        threshold = atof(argv[i]+sizeof("--print-bonds=")-1);
        hasThreshold = 1;
      } else if(!strncmp(argv[i],"--dg",sizeof("--dg")-1)) {
        float temp=C_to_kelvin(37), mg=0, cation=50, dNTP=0;
        char *q = argv[i]+sizeof("--dg")-1;
        if(*q=='=') {
          q++;
          char *r = strchr(q,','); if(r) {
            *r++=0;
            char *s = strchr(r,','); if(s) {
              *s++=0;
              char *t = strchr(s,','); if(t) {
                *t++=0; dNTP = atof(t);
              }
              cation = atof(s);
            }
            mg = atof(r);
          }
          temp = atof(q);
        }
        table = deltaG_table(temp,mg,cation,dNTP);
      }
      else if(!strncmp(argv[i],"--amp-max=",sizeof("--amp-max=")-1))
        maxAmpliconLen = atoi(argv[i]+sizeof("--amp-max=")-1);
      else if(!strncmp(argv[i],"--max-count=",sizeof("--max-count=")-1))
        maxCount = atoi(argv[i]+sizeof("--max-count=")-1);
      else if(!strcmp(argv[i],"--seedless"))
        seedless = 1;
      else if(!strcmp(argv[i],"--suggest-pools"))
        suggestPools = 1;
      else if(!strcmp(argv[i],"--scan-variants"))
        ignoreVars = 0;
      else if(!strcmp(argv[i],"--pools")) numPools = -1;
      else if(!strncmp(argv[i],"--pools=",sizeof("--pools=")-1)) {
        char *p = argv[i]+sizeof("--pools=")-1;
        char *q = strchr(p,','); if(q) {
          *q++=0;
          char *r=strchr(q,','); if(r) {
            *r=0; poolPrefix=r+1;
          }
          mins = atoi(q);
        } numPools = (*p=='?')?-1:atoi(p);
      } else if(!strncmp(argv[i],"--genome=",sizeof("--genome=")-1))
        genomeFile = open_or_exit(argv[i]+sizeof("--genome=")-1,"rb");
      else if(!strncmp(argv[i],"--multiplx=",sizeof("--multiplx=")-1))
        multiplxFile = open_or_exit(argv[i]+sizeof("--multiplx=")-1,"w");
      else {
        if(ap.np) fprintf(stderr,"Ignoring extra argument %s\n",argv[i]); /* TODO: merge?  separately process with all options (re-opening genomeFile)?  low priority */
        ap=loadFASTA(open_or_exit(argv[i],"rb"));
      }
    }
    if(ap.np > 0) { /* FASTA file loaded OK */
      if(doCounts) {
        if(selfOmit) {
          PS_cache cache=PS_precalc(ap,table,NULL,NULL,0);
          if(cache.scores) {
            int *pools=calloc(ap.np,sizeof(int)); /* all in pool 0 */
            if(!memFail(pools,_memFail)) {
              if(table) dGprintStats(ap,pools,cache.scores,stdout);
              else printStats(ap,pools,cache.scores,stdout);
              free(pools);
            }
            PS_free(cache);
          } /* else memFail will already have printed something */
        } else {
          addTags(ap);
          if(table) dGandScoreCounts(ap,table,stdout);
          else printCounts(ap,stdout);
          removeTags(ap);
        }
      }
      if(numPools>0 && maxCount) {
        /* preliminary: check range of maxCount */
        int average=2*averagePairsRoundUp(ap.np,numPools);
        if(maxCount<average) {
          fputs("Can't do that: --max-count is too low!\n",stderr); suggestMax(numPools,average,ap.np);
          exit(1); /* no need to free memory */
        }
      }
      if(numPools==1) { /* we don't want "division by zero" in pool-split.c */
        fputs("Cannot divide into fewer than 2 pools\n",stderr);
        exit(1); /* no need to free memory */
      }
      int nAmplicons=0; char *overlappingAmplicons=NULL; int *primerNoToAmpliconNo=NULL;
      if(genomeFile)
        overlappingAmplicons=GetOverlappingAmplicons(ap,genomeFile,&primerNoToAmpliconNo,&nAmplicons,maxAmpliconLen,multiplxFile,1 /* TODO: support 0 here for just an all-amplicon-locations-file from command line? */, ignoreVars);
      if(numPools || suggestPools) {
        PS_cache cache=PS_precalc(ap,table,overlappingAmplicons,primerNoToAmpliconNo,nAmplicons);
        if(suggestPools || numPools<0) {
          int suggestion = suggest_num_pools(ap,cache,table);
          fprintf(stderr,"Computer suggestion is %d pools.\n",suggestion);
          if(numPools<0) {
            numPools = suggestion;
            if (maxCount) {
              /* as the 'preliminary' above, but we now need to check it here (TODO: duplicate code) */
              int average=2*averagePairsRoundUp(ap.np,numPools);
              if(maxCount<average) {
                fputs("Can't do that: --max-count is too low!\n",stderr); suggestMax(numPools,average,ap.np);
                exit(1); /* no need to free memory */
              }
            }
          }
        }
        int *pools = NULL;
        if (numPools) pools=split_into_pools(ap,numPools,mins,cache,seedless,table,maxCount);
        PS_free(cache);
        if(pools) {
          if(hasThreshold) {
            addTags(ap);
            if(table) dGprintBonds(ap,stdout,threshold,pools,table);
            else printBonds(ap,stdout,threshold,pools);
            removeTags(ap);
          }
          if(poolPrefix) {
            if(strcmp(poolPrefix,"-")) pools2files(poolPrefix,pools,numPools,ap); else allPoolsToOneFile(stdout,pools,numPools,ap);
          }
          free(pools);
        }
      } else if(hasThreshold) {
        addTags(ap);
        if(table) dGprintBonds(ap,stdout,threshold,NULL,table);
        else printBonds(ap,stdout,threshold,NULL);
      }
      freeAllPrimers(ap);
      if(overlappingAmplicons) free(overlappingAmplicons);
      if(primerNoToAmpliconNo) free(primerNoToAmpliconNo);
    } if(table) free(table);
  } return 0;
}
