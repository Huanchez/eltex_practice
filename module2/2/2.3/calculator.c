#include "calculator.h"

#ifndef CALC_REG_CAP
#define CALC_REG_CAP 32
#endif

typedef struct { char sym; calc_op_fn fn; } reg_entry;

static reg_entry g_reg[CALC_REG_CAP];
static int g_reg_count = 0;

static int find_index(char sym){
    for(int i=0;i<g_reg_count;++i) if(g_reg[i].sym==sym) return i;
    return -1;
}

int calc_register(char sym, calc_op_fn fn){
    int idx = find_index(sym);
    if (idx >= 0) { g_reg[idx].fn = fn; return CALC_OK; }
    if (g_reg_count < CALC_REG_CAP) g_reg[g_reg_count++] = (reg_entry){sym, fn};
    return CALC_OK;
}

int calc_apply(char sym, double* ans, int argc, const double argv[]){
    int idx = find_index(sym);
    if (idx < 0) return CALC_ERR_NO_SUCH_OP;
    return g_reg[idx].fn(ans, argc, argv);
}

int calc_list(char *out_syms, int max_syms){
    if (!out_syms || max_syms<=0) return 0;
    int n = (g_reg_count < max_syms) ? g_reg_count : max_syms;
    for (int i=0;i<n;++i) out_syms[i]=g_reg[i].sym;
    return n;
}

int add(double* ans, int argc, const double argv[]){
    double s=0.0; for(int i=0;i<argc;++i) s+=argv[i]; *ans=s; return CALC_OK;
}

int subtract(double* ans, int argc, const double argv[]){
    if (argc<=0){ *ans=0.0; return CALC_OK; }
    double r=argv[0]; for(int i=1;i<argc;++i) r-=argv[i]; *ans=r; return CALC_OK;
}

int multiply(double* ans, int argc, const double argv[]){
    if (argc<=0){ *ans=0.0; return CALC_OK; }
    double p=1.0; for(int i=0;i<argc;++i) p*=argv[i]; *ans=p; return CALC_OK;
}

int divide(double* ans, int argc, const double argv[]){
    if (argc<=0){ *ans=0.0; return CALC_OK; }
    double q=argv[0];
    for(int i=1;i<argc;++i){ if(argv[i]==0.0) return CALC_ERR_DIV_ZERO; q/=argv[i]; }
    *ans=q; return CALC_OK;
}
