/*
# This file is part of Primer Pooler v1.43 (c) 2016-18 Silas S. Brown.  For Wen.
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
/* bit-common.c is the only file that's allowed to include
   bit-choice.h, expanding to the architecture-dependent
   choice of (32 + 64) or (64 + 128) bit functions.
   All other modules that need these functions should
   include all-primers.h instead.
*/
#include "bit-basics.h"
#include "triangle.h"
#include "64.h"
#if Has_128bit
#include "128.h"
#include "64-128.h"
#else
#if defined(__x86_64) || defined(__ARM_ARCH_ISA_A64)
#include "64-only.h"
#else
#include "32.h"
#include "32-64.h"
#endif
#endif
