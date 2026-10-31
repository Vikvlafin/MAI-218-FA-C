#ifndef FA2_BACKUP_H
#define FA2_BACKUP_H
#include <stddef.h>
#include <stdint.h>
typedef struct { char *name; size_t size; size_t importance; } BackupFile;
typedef struct {
    BackupFile *files;
    size_t count;
    size_t total_size;
    uintmax_t total_importance;
} BackupResult;
int choose_backup(const BackupFile *files, size_t count, size_t capacity, BackupResult *result);
#endif
