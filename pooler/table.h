/*
# This file is part of Primer Pooler v1.7 (c) 2016-20 Silas S. Brown.  For Wen.
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
/* Table generated by table.py */
typedef struct {
  float dH_kcal_per_mol;
  float dS_cal_per_molK; } dHS;
/* 8-bit offsets are AorTF1,AorTF2,GorTF1,GorTF2,AorTR1,AorTR2,GorTR1,GorTR2
   last 3 offsets are for corrections */
static const dHS dHStable[259]={
  /* CC/CC */ {0,0},
  /* CC/CG */ {3.6,8.9},
  /* CC/GC */ {-1.5,-7.2},
  /* CC/GG */ {-8.0,-19.9},
  /* CC/CA */ {0,0},
  /* CC/CT */ {0,0},
  /* CC/GA */ {0.6,-0.6},
  /* CC/GT */ {-0.8,-4.5},
  /* CC/AC */ {0,0},
  /* CC/AG */ {5.2,14.2},
  /* CC/TC */ {0,0},
  /* CC/TG */ {5.2,13.5},
  /* CC/AA */ {0,0},
  /* CC/AT */ {0,0},
  /* CC/TA */ {0,0},
  /* CC/TT */ {0,0},
  /* CG/CC */ {-1.5,-7.2},
  /* CG/CG */ {0,0},
  /* CG/GC */ {-10.6,-27.2},
  /* CG/GG */ {-4.9,-15.3},
  /* CG/CA */ {0,0},
  /* CG/CT */ {0,0},
  /* CG/GA */ {-4.0,-13.2},
  /* CG/GT */ {-4.1,-11.7},
  /* CG/AC */ {1.9,3.7},
  /* CG/AG */ {0,0},
  /* CG/TC */ {-1.5,-6.1},
  /* CG/TG */ {0,0},
  /* CG/AA */ {0,0},
  /* CG/AT */ {0,0},
  /* CG/TA */ {0,0},
  /* CG/TT */ {0,0},
  /* GC/CC */ {3.6,8.9},
  /* GC/CG */ {-9.8,-24.4},
  /* GC/GC */ {0,0},
  /* GC/GG */ {-6.0,-15.8},
  /* GC/CA */ {-0.7,-3.8},
  /* GC/CT */ {2.3,5.4},
  /* GC/GA */ {0,0},
  /* GC/GT */ {0,0},
  /* GC/AC */ {0,0},
  /* GC/AG */ {-0.6,-1.0},
  /* GC/TC */ {0,0},
  /* GC/TG */ {-4.4,-12.3},
  /* GC/AA */ {0,0},
  /* GC/AT */ {0,0},
  /* GC/TA */ {0,0},
  /* GC/TT */ {0,0},
  /* GG/CC */ {-8.0,-19.9},
  /* GG/CG */ {-6.0,-15.8},
  /* GG/GC */ {-4.9,-15.3},
  /* GG/GG */ {0,0},
  /* GG/CA */ {0.5,3.2},
  /* GG/CT */ {3.3,10.4},
  /* GG/GA */ {0,0},
  /* GG/GT */ {0,0},
  /* GG/AC */ {-0.7,-2.3},
  /* GG/AG */ {0,0},
  /* GG/TC */ {-2.8,-8.0},
  /* GG/TG */ {0,0},
  /* GG/AA */ {0,0},
  /* GG/AT */ {0,0},
  /* GG/TA */ {0,0},
  /* GG/TT */ {5.8,16.3},
  /* CA/CC */ {0,0},
  /* CA/CG */ {0,0},
  /* CA/GC */ {1.9,3.7},
  /* CA/GG */ {-0.7,-2.3},
  /* CA/CA */ {0,0},
  /* CA/CT */ {6.1,16.4},
  /* CA/GA */ {-0.9,-4.2},
  /* CA/GT */ {-8.5,-22.7},
  /* CA/AC */ {0,0},
  /* CA/AG */ {0,0},
  /* CA/TC */ {0,0},
  /* CA/TG */ {0,0},
  /* CA/AA */ {0,0},
  /* CA/AT */ {3.4,8.0},
  /* CA/TA */ {0,0},
  /* CA/TT */ {1.0,0.7},
  /* CT/CC */ {0,0},
  /* CT/CG */ {0,0},
  /* CT/GC */ {-1.5,-6.1},
  /* CT/GG */ {-2.8,-8.0},
  /* CT/CA */ {0.0,-4.4},
  /* CT/CT */ {0,0},
  /* CT/GA */ {-7.8,-21.0},
  /* CT/GT */ {-5.0,-15.8},
  /* CT/AC */ {0,0},
  /* CT/AG */ {0,0},
  /* CT/TC */ {0,0},
  /* CT/TG */ {0,0},
  /* CT/AA */ {2.3,4.6},
  /* CT/AT */ {0,0},
  /* CT/TA */ {-1.2,-6.2},
  /* CT/TT */ {0,0},
  /* GA/CC */ {5.2,14.2},
  /* GA/CG */ {-0.6,-1.0},
  /* GA/GC */ {0,0},
  /* GA/GG */ {0,0},
  /* GA/CA */ {-2.9,-9.8},
  /* GA/CT */ {-8.2,-22.2},
  /* GA/GA */ {0,0},
  /* GA/GT */ {1.6,3.6},
  /* GA/AC */ {0,0},
  /* GA/AG */ {0,0},
  /* GA/TC */ {0,0},
  /* GA/TG */ {0,0},
  /* GA/AA */ {0,0},
  /* GA/AT */ {0.7,0.7},
  /* GA/TA */ {0,0},
  /* GA/TT */ {-1.3,-5.3},
  /* GT/CC */ {5.2,13.5},
  /* GT/CG */ {-4.4,-12.3},
  /* GT/GC */ {0,0},
  /* GT/GG */ {0,0},
  /* GT/CA */ {-8.4,-22.4},
  /* GT/CT */ {-2.2,-8.4},
  /* GT/GA */ {-3.1,-9.5},
  /* GT/GT */ {0,0},
  /* GT/AC */ {0,0},
  /* GT/AG */ {0,0},
  /* GT/TC */ {0,0},
  /* GT/TG */ {4.1,9.5},
  /* GT/AA */ {-0.6,-2.3},
  /* GT/AT */ {0,0},
  /* GT/TA */ {-2.5,-8.3},
  /* GT/TT */ {0,0},
  /* AC/CC */ {0,0},
  /* AC/CG */ {-0.7,-3.8},
  /* AC/GC */ {0,0},
  /* AC/GG */ {0.5,3.2},
  /* AC/CA */ {0,0},
  /* AC/CT */ {0,0},
  /* AC/GA */ {0,0},
  /* AC/GT */ {0,0},
  /* AC/AC */ {0,0},
  /* AC/AG */ {-2.9,-9.8},
  /* AC/TC */ {0.0,-4.4},
  /* AC/TG */ {-8.4,-22.4},
  /* AC/AA */ {0,0},
  /* AC/AT */ {0,0},
  /* AC/TA */ {5.3,14.6},
  /* AC/TT */ {0.7,0.2},
  /* AG/CC */ {0.6,-0.6},
  /* AG/CG */ {0,0},
  /* AG/GC */ {-4.0,-13.2},
  /* AG/GG */ {0,0},
  /* AG/CA */ {0,0},
  /* AG/CT */ {0,0},
  /* AG/GA */ {0,0},
  /* AG/GT */ {0,0},
  /* AG/AC */ {-0.9,-4.2},
  /* AG/AG */ {0,0},
  /* AG/TC */ {-7.8,-21.0},
  /* AG/TG */ {-3.1,-9.5},
  /* AG/AA */ {0,0},
  /* AG/AT */ {0,0},
  /* AG/TA */ {-0.7,-2.3},
  /* AG/TT */ {1.0,0.9},
  /* TC/CC */ {0,0},
  /* TC/CG */ {2.3,5.4},
  /* TC/GC */ {0,0},
  /* TC/GG */ {3.3,10.4},
  /* TC/CA */ {0,0},
  /* TC/CT */ {0,0},
  /* TC/GA */ {0,0},
  /* TC/GT */ {0,0},
  /* TC/AC */ {6.1,16.4},
  /* TC/AG */ {-8.2,-22.2},
  /* TC/TC */ {0,0},
  /* TC/TG */ {-2.2,-8.4},
  /* TC/AA */ {7.6,20.2},
  /* TC/AT */ {1.2,0.7},
  /* TC/TA */ {0,0},
  /* TC/TT */ {0,0},
  /* TG/CC */ {-0.8,-4.5},
  /* TG/CG */ {0,0},
  /* TG/GC */ {-4.1,-11.7},
  /* TG/GG */ {0,0},
  /* TG/CA */ {0,0},
  /* TG/CT */ {0,0},
  /* TG/GA */ {0,0},
  /* TG/GT */ {-1.4,-6.2},
  /* TG/AC */ {-8.5,-22.7},
  /* TG/AG */ {1.6,3.6},
  /* TG/TC */ {-5.0,-15.8},
  /* TG/TG */ {0,0},
  /* TG/AA */ {3.0,7.4},
  /* TG/AT */ {-0.1,-1.7},
  /* TG/TA */ {0,0},
  /* TG/TT */ {0,0},
  /* AA/CC */ {0,0},
  /* AA/CG */ {0,0},
  /* AA/GC */ {0,0},
  /* AA/GG */ {0,0},
  /* AA/CA */ {0,0},
  /* AA/CT */ {7.6,20.2},
  /* AA/GA */ {0,0},
  /* AA/GT */ {3.0,7.4},
  /* AA/AC */ {0,0},
  /* AA/AG */ {0,0},
  /* AA/TC */ {2.3,4.6},
  /* AA/TG */ {-0.6,-2.3},
  /* AA/AA */ {0,0},
  /* AA/AT */ {4.7,12.9},
  /* AA/TA */ {1.2,1.7},
  /* AA/TT */ {-7.9,-22.2},
  /* AT/CC */ {0,0},
  /* AT/CG */ {0,0},
  /* AT/GC */ {0,0},
  /* AT/GG */ {0,0},
  /* AT/CA */ {5.3,14.6},
  /* AT/CT */ {0,0},
  /* AT/GA */ {-0.7,-2.3},
  /* AT/GT */ {0,0},
  /* AT/AC */ {0,0},
  /* AT/AG */ {0,0},
  /* AT/TC */ {-1.2,-6.2},
  /* AT/TG */ {-2.5,-8.3},
  /* AT/AA */ {1.2,1.7},
  /* AT/AT */ {0,0},
  /* AT/TA */ {-7.2,-20.4},
  /* AT/TT */ {-2.7,-10.8},
  /* TA/CC */ {0,0},
  /* TA/CG */ {0,0},
  /* TA/GC */ {0,0},
  /* TA/GG */ {0,0},
  /* TA/CA */ {0,0},
  /* TA/CT */ {1.2,0.7},
  /* TA/GA */ {0,0},
  /* TA/GT */ {-0.1,-1.7},
  /* TA/AC */ {3.4,8.0},
  /* TA/AG */ {0.7,0.7},
  /* TA/TC */ {0,0},
  /* TA/TG */ {0,0},
  /* TA/AA */ {4.7,12.9},
  /* TA/AT */ {-7.2,-21.3},
  /* TA/TA */ {0,0},
  /* TA/TT */ {0.2,-1.5},
  /* TT/CC */ {0,0},
  /* TT/CG */ {0,0},
  /* TT/GC */ {0,0},
  /* TT/GG */ {5.8,16.3},
  /* TT/CA */ {0.7,0.2},
  /* TT/CT */ {0,0},
  /* TT/GA */ {1.0,0.9},
  /* TT/GT */ {0,0},
  /* TT/AC */ {1.0,0.7},
  /* TT/AG */ {-1.3,-5.3},
  /* TT/TC */ {0,0},
  /* TT/TG */ {0,0},
  /* TT/AA */ {-7.9,-22.2},
  /* TT/AT */ {0.2,-1.5},
  /* TT/TA */ {-2.7,-10.8},
  /* TT/TT */ {0,0},
  /* [256] = init C or G */ { 0.1,-2.8},
  /* [257] = init A or T */ { 2.3,4.1},
  /* [258] = symmetry correction TODO: how to use this? (MPprimer doesn't) */ { 0,-1.4},
};
