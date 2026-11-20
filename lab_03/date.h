#ifndef FA3_DATE_H
#define FA3_DATE_H
#include <time.h>

static inline int calendar(int day_of_month, int month, int year_number, uint64_t *out) {
    if (!out || year_number < 1 || year_number > 9999 || month < 1 || month > 12 ||
        day_of_month < 1) {
        return 1;
    }
    const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int leap = year_number % 4 == 0 && (year_number % 100 != 0 || year_number % 400 == 0);
    if (day_of_month > days[month - 1] + (month == 2 && leap)) {
        return 1;
    }
    uint64_t year = (uint64_t)year_number - 1;
    uint64_t total = year * 365 + year / 4 - year / 100 + year / 400;
    for (int i = 1; i < month; ++i) {
        total += (unsigned)(days[i - 1] + (i == 2 && leap));
    }
    *out = (total + (unsigned)day_of_month - 1) * 86400;
    return 0;
}

static inline int timestamp(const char *date, const char *time, char sep, uint64_t *out) {
    if (!date || !out || strlen(date) != 10 || date[2] != sep || date[5] != sep) {
        return 1;
    }
    for (size_t i = 0; i < 10; ++i) {
        if (i != 2 && i != 5 && !isdigit((unsigned char)date[i])) {
            return 1;
        }
    }
    int day_of_month = atoi(date);
    int month = atoi(date + 3);
    int year = atoi(date + 6);
    uint64_t day;
    if (calendar(day_of_month, month, year, &day)) {
        return 1;
    }
    if (time) {
        if (strlen(time) != 8 || time[2] != ':' || time[5] != ':') {
            return 1;
        }
        for (size_t i = 0; i < 8; ++i) {
            if (i != 2 && i != 5 && !isdigit((unsigned char)time[i])) {
                return 1;
            }
        }
        int hours = atoi(time);
        int minutes = atoi(time + 3);
        int seconds = atoi(time + 6);
        if (hours > 23 || minutes > 59 || seconds > 59) {
            return 1;
        }
        day += (unsigned)(hours * 3600 + minutes * 60 + seconds);
    }
    *out = day;
    return 0;
}

static inline int latin(const char *s) {
    if (!s || !*s) {
        return 1;
    }
    for (; *s; ++s) {
        if (!((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z'))) {
            return 1;
        }
    }
    return 0;
}
#endif
