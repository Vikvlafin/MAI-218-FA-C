#include "functions.h"
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

int shift_polynomial(double epsilon, double a, double **result, int degree, ...) {
    if (!result || !isfinite(epsilon) || epsilon<=0 || !isfinite(a) || degree<0 ||
        (size_t)degree>=SIZE_MAX/sizeof(double)) return 1;
    size_t n=(size_t)degree+1;
    double *g=malloc(n*sizeof(*g));
    if (!g) return 2;
    va_list args; va_start(args,degree);
    int status=0;
    for (size_t i=0; i<n; ++i) {
        g[i]=va_arg(args,double);
        if (!isfinite(g[i])) status=1;
    }
    va_end(args);
    for (size_t step=0; !status && step<(size_t)degree; ++step) {
        for (size_t j=(size_t)degree; j>step; --j) {
            size_t i=j-1;
            long double value=(long double)g[i]+(long double)a*g[i+1];
            if (!isfinite(value) || fabsl(value)>DBL_MAX) { status=3; break; }
            double rounded=(double)value;
            if (fabsl(value-rounded)>(long double)epsilon*fmaxl(1.0L,fabsl(value))) { status=3; break; }
            g[i]=rounded;
        }
    }
    if (!status) {
        *result=g;
    } else free(g);
    return status;
}
