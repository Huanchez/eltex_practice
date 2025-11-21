#include "calculator.h"
#include <stdarg.h>

int add(double* ans, int count, ...) {
    va_list args; 
    va_start(args, count);
    if (!ans) { va_end(args); return CALC_ERR_BAD_PTR; }

    *ans = 0.0;
    for (int i = 0; i < count; ++i) {
        *ans += va_arg(args, double);
    }
    va_end(args);
    return CALC_OK;
}

int subtract(double* ans, int count, ...) {
    va_list args; 
    va_start(args, count);
    if (!ans) { va_end(args); return CALC_ERR_BAD_PTR; }

    *ans = (count > 0) ? va_arg(args, double) : 0.0;
    for (int i = 1; i < count; ++i) {
        *ans -= va_arg(args, double);
    }
    va_end(args);
    return CALC_OK;
}

int multiply(double* ans, int count, ...) {
    va_list args; 
    va_start(args, count);
    if (!ans) { va_end(args); return CALC_ERR_BAD_PTR; }

    *ans = 1.0;
    for (int i = 0; i < count; ++i) {
        *ans *= va_arg(args, double);
    }
    va_end(args);
    return CALC_OK;
}

int divide(double* ans, int count, ...) {
    va_list args; 
    va_start(args, count);
    if (!ans) { va_end(args); return CALC_ERR_BAD_PTR; }

    *ans = (count > 0) ? va_arg(args, double) : 0.0;
    for (int i = 1; i < count; ++i) {
        double d = va_arg(args, double);
        if (d == 0.0) { va_end(args); return CALC_ERR_DIV_ZERO; }
        *ans /= d;
    }
    va_end(args);
    return CALC_OK;
}
