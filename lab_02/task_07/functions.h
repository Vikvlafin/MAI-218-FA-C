#ifndef FA2_NORMS_H
#define FA2_NORMS_H
#include <stddef.h>
typedef struct { const double *coordinates; } Vector;
typedef int (*NormFunction)(const Vector *, size_t, const void *, double, double *);
typedef struct { NormFunction function; const void *context; } NormSpec;
typedef struct { Vector *vectors; size_t count; } NormResult;
int norm_infinity(const Vector *vector, size_t n, const void *context, double epsilon, double *result);
int norm_p(const Vector *vector, size_t n, const void *context, double epsilon, double *result);
int norm_matrix(const Vector *vector, size_t n, const void *context, double epsilon, double *result);
int longest_vectors(size_t n, double epsilon, NormResult **result, size_t norm_count, size_t vector_count, ...);
int free_norm_results(NormResult *results, size_t count);
#endif
