#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stddef.h>

typedef struct StringBuffer {
    char *data;
    size_t len;
    size_t cap;
} StringBuffer;

StringBuffer sbCreate(size_t initCap);
void sbFree(StringBuffer *sb);
void sbAppend(StringBuffer *sb, const char *str);
void sbAppendf(StringBuffer *sb, const char *fmt, ...);
void sbAppendChar(StringBuffer *sb, char c);
void sbEnsureCapacity(StringBuffer *sb, size_t needed);

#endif // STRING_BUFFER_H