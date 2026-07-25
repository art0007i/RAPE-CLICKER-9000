#ifndef LICENSES_H
#define LICENSES_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef struct {
    const char *lib;
    const char *url;
    const char *license;
} license;

size_t get_license_count(void);
const license *get_licenses(void);
const license *get_own_license(void);

#ifdef __cplusplus
}
#endif
#endif // LICENSES_H
