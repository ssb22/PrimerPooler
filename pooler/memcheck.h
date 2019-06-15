/*
# This file is part of Primer Pooler v1.6 (c) 2016-19 Silas S. Brown.  For Wen.
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
int _memFail(void *first,...);
#define memFail(...) _memFail(__FILE__, __LINE__, __VA_ARGS__)
static inline void* memTrim(void *p,void *top) {
  void *r = realloc(p,top-p);
  return r ? r : p;
}
