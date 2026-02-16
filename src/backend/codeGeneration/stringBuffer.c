#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "stringBuffer.h"

StringBuffer sbCreate(size_t init){
    StringBuffer sb = {0};
    sb.cap = init > 0 ? init : 1024;
    sb.data = malloc(sb.cap);
    if(sb.data){
        sb.data[0] = '\0';
        sb.len = 0;
    }
    return sb;
}

void sbFree(StringBuffer *sb) {
    if (sb && sb->data) {
        free(sb->data);
        sb->data = NULL;
        sb->len = 0;
        sb->cap = 0;
    }
}

void sbEnsureCapacity(StringBuffer *sb, size_t needed) {
    if (sb->len + needed + 1 > sb->cap) {
        size_t newCap = sb->cap * 2;
        while (newCap < sb->len + needed + 1) {
            newCap *= 2;
        }
        char *newData = realloc(sb->data, newCap);
        if (newData) {
            sb->data = newData;
            sb->cap = newCap;
        }
    }
}

void sbAppendf(StringBuffer *sb, const char *fmt, ...) {
    if (!sb || !fmt) return;
    
    va_list args;
    va_start(args, fmt);
    
    va_list argsCopy;
    va_copy(argsCopy, args);
    int needed = vsnprintf(NULL, 0, fmt, argsCopy);
    va_end(argsCopy);
    
    if (needed < 0) {
        va_end(args);
        return;
    }
    
    sbEnsureCapacity(sb, (size_t)needed);
    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, args);
    sb->len += needed;
    
    va_end(args);
}

void sbAppend (StringBuffer *sb, const char *str){
    if(!sb || !str) return;
    size_t len = strlen(str);
    sbEnsureCapacity(sb, len);
    memcpy(sb->data + sb ->len, str, len);
    sb->len += len;
    sb->data[sb->len] = '\0';
}

void sbAppendChar(StringBuffer *sb, char c) {
    if (!sb) return;
    sbEnsureCapacity(sb, 1);
    sb->data[sb->len++] = c;
    sb->data[sb->len] = '\0';
}