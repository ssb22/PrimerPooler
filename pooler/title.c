/*
# This file is part of Primer Pooler v1.4 (c) 2016-17 Silas S. Brown.  For Wen.
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
#include <string.h>
#include "ansi.h"
#include "copyright.h"
void printTitle() {
#if __LP64__
#define CpuBits "64"
#define ExtraLine1 ""
#else
#define CpuBits "32"
#if defined(_WIN32)
  // you're not running the 32-bit .exe on 64-bit are you?
#define ExtraLine1 "If you have a 64-bit CPU, the 64-bit version is faster.\n"
#else
/* non-Windows non-64bit probably means user's CPU cannot
   do 64-bit (and they know it's not the latest) so no
   need to say anything */
#define ExtraLine1 ""
#endif
#endif
#if _OPENMP
#define CoreType "multi-core"
#define ExtraLine2 ""
#else
#define CoreType "single-core"
#define ExtraLine2 "You might wish to update your compiler if you want the multi-core version.\n"
#endif
  char title[80];
  snprintf(title,sizeof(title),"%.*s" CpuBits "-bit " CoreType,(int)(strchr(copyright,'(')-copyright),copyright);
  ConsoleTitle(title);
  SetBold(); fputs(copyright,stderr); ResetColour();
  fprintf(stderr,"\nCompiled on " __DATE__ " for " CpuBits "-bit " CoreType " CPUs.\n" ExtraLine1 ExtraLine2 "\n");
}
