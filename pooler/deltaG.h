/*
# This file is part of Primer Pooler v1.42 (c) 2016-18 Silas S. Brown.  For Wen.
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
#include <math.h>
#include "table.h"
static inline float C_to_kelvin(float C) { return C+273.15; }
static inline float F_to_kelvin(float F) { return (F+459.67)*5.0/9.0; }
static inline float R_to_kelvin(float R) { return R*5.0/9.0; }
static inline float E_to_kelvin(float E) { return C_to_kelvin((E-16.0)*5.0/7.0); }
static float* deltaG_table(float K,float mg_conc_mM,float monovalent_cation_conc_mM,float dNTP_conc_mM) {
  // MPprimer has monovalent_cation_conc_mM default 50, others 0
  // Versions prior to 1.17 incorrectly called these _nM instead of _mM (unit, you nit :-) )
  float* r=malloc(259*sizeof(float)); if(!r) return NULL;
  int i;
  float entropy_adjust = (0.368 * log((monovalent_cation_conc_mM + 120 * ((mg_conc_mM>dNTP_conc_mM) ? sqrt(mg_conc_mM-dNTP_conc_mM) : 0))/1000));
  
  for(i=0; i<259; i++)
    r[i] = dHStable[i].dH_kcal_per_mol
      - (dHStable[i].dS_cal_per_molK+entropy_adjust)*K/1000.0;
  return r;
}
