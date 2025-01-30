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
#if CPU_64bit
#include "64-only.h"
#else
#include "32.h"
#include "32-64.h"
#endif
#endif
