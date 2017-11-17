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
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#if defined(_WIN64) || defined(_WIN32)
/* we'll emulate some of ANSI on the Windows console
   (not all versions of Windows can interpret ANSI) */
#include <windows.h>
#else
#include <unistd.h>
#endif
static int CanDoANSI() {
#if defined(_WIN64) || defined(_WIN32)
  return 0; /* don't try to read TERM: fails on some WINE versions, which return Unix environment variables even when running in wineconsole */
#else
  if(!isatty(2)) return 0;
  const char *term = getenv("TERM");
  return term && (strstr(term,"xterm") || !strcmp(term,"screen") || !strcmp(term,"linux"));
#endif
}
static inline char *clearEOL() {
  return CanDoANSI() ? "\x1b[K" : "         ";
}
#if defined(_WIN64) || defined(_WIN32)
/* Microsoft order: RGB in bits 4,2,1 */
enum { Black, Blue, Green, Cyan, Red, Magenta, Yellow, White };
#else
/* ANSI order: BGR in bits 4,2,1 */
enum { Black, Red, Green, Yellow, Blue, Magenta, Cyan, White };
#endif
enum { Dark, Bright };
static inline void SetColour(int intensity,int fg,int bg) {
#if defined(_WIN64) || defined(_WIN32)
  SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE),fg | (intensity<<3) | (bg<<4));
#else
  if(CanDoANSI()) fprintf(stderr,"\x1b[%d;%d%sm",fg+30,bg+40,intensity?";1":"");
#endif
} static inline void ResetColour() {
#if defined(_WIN64) || defined(_WIN32)
  SetColour(Dark,White,Black);
#else
  if(CanDoANSI()) fputs("\x1b[0m",stderr);
#endif
} static inline void SetBold() {
#if defined(_WIN64) || defined(_WIN32)
  /* On Unix terminals, we don't know what the background
     colour is or what the normal foreground colour is,
     and it's better if we don't try to set the background
     so we have to make sure all our foreground colours
     are vaguely compatible with the normal selections and
     just have ANSI bold for bold.  But on Windows console
     the standard background is black like DOS. */
  SetColour(Bright,Green,Black); /* might work better than bright white */
#else
  if(CanDoANSI()) fputs("\x1b[1m",stderr);
#endif
}
static void ClearConsoleTitle();
static void CCT_on_Signal(int sig) {
  /* hopefully OK if re-entered by other signals;
     worst-case scenario is minor display corruption */
  ClearConsoleTitle();
  signal(sig,SIG_DFL); raise(sig);
}
static inline void CT_SetSignals() {
  atexit(ClearConsoleTitle);
  int i;
  for(i=1; i<=15; i++) signal(i, CCT_on_Signal);
  for(i=24; i<=27; i++) signal(i, CCT_on_Signal);
}
static inline void ConsoleTitle(const char *t) {
#if defined(_WIN64) || defined(_WIN32)
  #ifdef UNICODE
  /* TODO: convert t to wide chars */
  #else
  SetConsoleTitle(t);
  if(*t) CT_SetSignals();
  #endif
#else
  if(CanDoANSI()) {
    fprintf(stderr,"\033]0;%s\007",t);
    if(*t) CT_SetSignals();
  }
#endif
}
static void ClearConsoleTitle() { ConsoleTitle(""); }
