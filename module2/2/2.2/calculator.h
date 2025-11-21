#pragma once

enum {
    CALC_OK = 0,
    CALC_ERR_DIV_ZERO = -1,
    CALC_ERR_BAD_PTR  = -2
};

int add      (double* ans, int count, ...);
int subtract (double* ans, int count, ...);
int multiply (double* ans, int count, ...);
int divide   (double* ans, int count, ...);
