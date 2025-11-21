#pragma once

enum {
    CALC_OK = 0,
    CALC_ERR_DIV_ZERO = -1,
    CALC_ERR_NO_SUCH_OP = -2
};

typedef int (*calc_op_fn)(double* ans, int argc, const double argv[]);

int calc_register(char sym, calc_op_fn fn);
int calc_apply(char sym, double* ans, int argc, const double argv[]);
int calc_list(char *out_syms, int max_syms);

int add(double* ans, int argc, const double argv[]);
int subtract(double* ans, int argc, const double argv[]);
int multiply(double* ans, int argc, const double argv[]);
int divide(double* ans, int argc, const double argv[]);
