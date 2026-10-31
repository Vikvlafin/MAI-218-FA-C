#ifndef FA2_EMPLOYEE_H
#define FA2_EMPLOYEE_H
#include <stddef.h>
#include <stdio.h>
typedef struct {
    size_t id;
    char *name;
    char *surname;
    char *storage;
    double salary;
    long double salary_group;
} Employee;
int read_employees(FILE *file, double epsilon, Employee **result, size_t *count);
int sort_employees(Employee *employees, size_t count, int descending);
int write_employees(FILE *file, const Employee *employees, size_t count);
int free_employees(Employee *employees, size_t count);
#endif
