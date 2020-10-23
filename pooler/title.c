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
#include "ansi.h"
#include "version.h"
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
  ConsoleTitle(Program_Version " " CpuBits "-bit " CoreType);
  SetBold(); fputs(Program_Version " " Program_Copyright,stderr); ResetColour();
  fprintf(stderr,"\nNow licensed under the Apache License, Version 2.0\nCompiled on " __DATE__ " for " CpuBits "-bit " CoreType " CPUs.\n" ExtraLine1 ExtraLine2 "\n");
}
