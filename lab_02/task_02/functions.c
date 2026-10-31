#include "functions.h"
#include "../input.h"
#include <string.h>

static int validate_name(const char *s) {
    if (!s || !*s) return 1;
    for (size_t i=0; s[i]; ++i)
        if (!((s[i]>='a' && s[i]<='z') || (s[i]>='A' && s[i]<='Z'))) return 1;
    return 0;
}

int free_employees(Employee *employees, size_t count) {
    if (!employees && count) return 1;
    if (employees) for (size_t i=0; i<count; ++i) free(employees[i].storage);
    free(employees);
    return 0;
}

int read_employees(FILE *file, double epsilon, Employee **result, size_t *count) {
    if (!file || !result || !count || !isfinite(epsilon) || epsilon<=0) return 1;
    Employee *items=NULL;
    size_t n=0,capacity=0;
    int status=0;
    for (;;) {
        char *line=NULL,*fields[4];
        int read_status=read_line(file,&line);
        if (read_status==4) break;
        if (read_status) { status=read_status; break; }
        Employee item={0};
        if (split_fields(line,fields,4) || parse_size(fields[0],&item.id) ||
            validate_name(fields[1]) || validate_name(fields[2]) ||
            parse_real(fields[3],&item.salary) || item.salary<0) {
            free(line); status=1; break;
        }
        item.name=fields[1]; item.surname=fields[2]; item.storage=line;
        item.salary_group=floorl((long double)item.salary/epsilon);
        if (!isfinite(item.salary_group) || n==SIZE_MAX/sizeof(*items)) {
            free(line); status=1; break;
        }
        if (n==capacity) {
            size_t next_capacity=capacity ? capacity*2 : 16;
            if (capacity>SIZE_MAX/2 || next_capacity>SIZE_MAX/sizeof(*items)) {
                free(line); status=2; break;
            }
            Employee *next=realloc(items,next_capacity*sizeof(*items));
            if (!next) { free(line); status=2; break; }
            items=next; capacity=next_capacity;
        }
        items[n++]=item;
    }
    if (status) { if (free_employees(items,n)) status=1; }
    else { *result=items; *count=n; }
    return status;
}

static int compare_ascending(const void *left, const void *right) {
    const Employee *a=left,*b=right;
    if (a->salary_group!=b->salary_group) return a->salary_group>b->salary_group ? 1 : -1;
    int cmp=strcmp(a->surname,b->surname);
    if (cmp) return cmp>0 ? 1 : -1;
    cmp=strcmp(a->name,b->name);
    if (cmp) return cmp>0 ? 1 : -1;
    return (a->id>b->id)-(a->id<b->id);
}

static int compare_descending(const void *left, const void *right) {
    return -compare_ascending(left,right);
}

int sort_employees(Employee *employees, size_t count, int descending) {
    if ((!employees && count) || (descending!=0 && descending!=1)) return 1;
    for (size_t i=0; i<count; ++i)
        if (validate_name(employees[i].name) || validate_name(employees[i].surname) ||
            !isfinite(employees[i].salary) || employees[i].salary<0 ||
            !isfinite(employees[i].salary_group) || employees[i].salary_group<0) return 1;
    if (count>1) qsort(employees,count,sizeof(*employees),descending ? compare_descending : compare_ascending);
    return 0;
}

int write_employees(FILE *file, const Employee *employees, size_t count) {
    if (!file || (!employees && count)) return 1;
    for (size_t i=0; i<count; ++i) {
        if (validate_name(employees[i].name) || validate_name(employees[i].surname) ||
            !isfinite(employees[i].salary) || employees[i].salary<0) return 1;
        if (fprintf(file,"%zu %s %s %.17g\n",employees[i].id,employees[i].name,
                    employees[i].surname,employees[i].salary)<0) return 3;
    }
    return 0;
}
