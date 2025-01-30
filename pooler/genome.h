typedef uint32_t b32;
typedef char SeqName[256];
SeqName* go_through_genome(FILE *f,int ignoreVars);
void output_genome_segment(FILE *f,int targetRenumberedSeqNo,b32 baseStart,int nBases,FILE *out,int ignoreVars);
