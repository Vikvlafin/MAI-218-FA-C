#include "functions.h"
#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

static int validate_vector(const Vector *vector, size_t n, double epsilon, double *result) {
    if (!vector || !vector->coordinates || !n || !result || !isfinite(epsilon) || epsilon<=0) return 1;
    for (size_t i=0; i<n; ++i) if (!isfinite(vector->coordinates[i])) return 1;
    return 0;
}

int norm_infinity(const Vector *vector, size_t n, const void *context, double epsilon, double *result) {
    (void)context;
    if (validate_vector(vector,n,epsilon,result)) return 1;
    double maximum=0;
    for (size_t i=0; i<n; ++i) maximum=fmax(maximum,fabs(vector->coordinates[i]));
    *result=maximum;
    return 0;
}

int norm_p(const Vector *vector, size_t n, const void *context, double epsilon, double *result) {
    if (validate_vector(vector,n,epsilon,result) || !context) return 1;
    double p=*(const double *)context;
    if (!isfinite(p) || p<1) return 1;
    double scale=0;
    int status=norm_infinity(vector,n,NULL,epsilon,&scale);
    if (status) return status;
    if (scale==0) { *result=0; return 0; }
    long double sum=0;
    for (size_t i=0; i<n; ++i) sum+=powl(fabsl((long double)vector->coordinates[i]/scale),p);
    long double value=(long double)scale*powl(sum,1.0L/p);
    if (!isfinite(value) || value>DBL_MAX) return 3;
    *result=(double)value;
    return 0;
}

int norm_matrix(const Vector *vector, size_t n, const void *context, double epsilon, double *result) {
    if (validate_vector(vector,n,epsilon,result) || !context || n>SIZE_MAX/n ||
        n*n>SIZE_MAX/sizeof(long double)) return 1;
    const double *a=context;
    long double *l=calloc(n*n,sizeof(*l));
    if (!l) return 2;
    int status=0;
    for (size_t i=0; i<n && !status; ++i) {
        for (size_t j=0; j<=i; ++j) {
            if (!isfinite(a[i*n+j]) || !isfinite(a[j*n+i]) ||
                fabsl((long double)a[i*n+j]-a[j*n+i])>epsilon) { status=1; break; }
            long double value=((long double)a[i*n+j]+a[j*n+i])/2;
            for (size_t k=0; k<j; ++k) value-=l[i*n+k]*l[j*n+k];
            if (i==j) {
                if (!isfinite(value) || value<=0) { status=1; break; }
                l[i*n+j]=sqrtl(value);
            } else l[i*n+j]=value/l[j*n+j];
            if (!isfinite(l[i*n+j])) { status=3; break; }
        }
    }
    long double total=0;
    if (!status) {
        for (size_t j=0; j<n; ++j) {
            long double component=0;
            for (size_t i=j; i<n; ++i) component+=l[i*n+j]*vector->coordinates[i];
            total=hypotl(total,component);
        }
        if (!isfinite(total) || total>DBL_MAX) status=3;
        else *result=(double)total;
    }
    free(l);
    return status;
}

int free_norm_results(NormResult *results, size_t count) {
    if (!results && count) return 1;
    if (results) for (size_t i=0; i<count; ++i) free(results[i].vectors);
    free(results);
    return 0;
}

int longest_vectors(size_t n, double epsilon, NormResult **result, size_t norm_count, size_t vector_count, ...) {
    if (!n || !result || !norm_count || !vector_count || !isfinite(epsilon) || epsilon<=0 ||
        vector_count>SIZE_MAX/sizeof(Vector) || vector_count>SIZE_MAX/sizeof(double) ||
        norm_count>SIZE_MAX/sizeof(NormResult)) return 1;
    Vector *vectors=malloc(vector_count*sizeof(*vectors));
    double *values=malloc(vector_count*sizeof(*values));
    NormResult *out=calloc(norm_count,sizeof(*out));
    if (!vectors || !values || !out) { free(vectors); free(values); free(out); return 2; }
    va_list args; va_start(args,vector_count);
    int status=0;
    for (size_t i=0; i<vector_count; ++i) {
        vectors[i]=va_arg(args,Vector);
        if (validate_vector(&vectors[i],n,epsilon,&values[i])) status=1;
    }
    for (size_t j=0; j<norm_count && !status; ++j) {
        NormSpec spec=va_arg(args,NormSpec);
        if (!spec.function) { status=1; break; }
        double maximum=0;
        for (size_t i=0; i<vector_count; ++i) {
            status=spec.function(&vectors[i],n,spec.context,epsilon,&values[i]);
            if (status) break;
            if (!isfinite(values[i]) || values[i]<0) { status=1; break; }
            maximum=fmax(maximum,values[i]);
        }
        if (status) break;
        out[j].vectors=malloc(vector_count*sizeof(Vector));
        if (!out[j].vectors) { status=2; break; }
        for (size_t i=0; i<vector_count; ++i)
            if (fabs(values[i]-maximum)<=epsilon) out[j].vectors[out[j].count++]=vectors[i];
    }
    va_end(args); free(vectors); free(values);
    if (status) { if (free_norm_results(out,norm_count)) status=1; }
    else *result=out;
    return status;
}
