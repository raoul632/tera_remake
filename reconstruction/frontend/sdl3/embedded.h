#ifndef TERA_EMBEDDED_H
#define TERA_EMBEDDED_H
#include <stddef.h>
/* Generated read-only storage. No filesystem lookup or startup extraction.
 * Returns NULL for an unknown resource; size receives zero in that case. */
const unsigned char *tera_embedded_resource(const char *name,size_t *size);
/* NULL directory selects embedded storage. A supplied directory is an explicit,
 * strict development override: missing files fail, without fallback. */
int tera_resource_read(const char *directory,const char *name,unsigned char *out,
    size_t size,char *error,size_t capacity);
#endif
