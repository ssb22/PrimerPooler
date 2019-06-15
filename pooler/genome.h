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
typedef uint32_t b32;
typedef char SeqName[256];
SeqName* go_through_genome(FILE *f);
void output_genome_segment(FILE *f,int targetRenumberedSeqNo,b32 baseStart,int nBases,FILE *out);
