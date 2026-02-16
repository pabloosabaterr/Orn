#include <string.h>
#include <ctype.h>
#include <parser.h>

int parseInt(const char *start, size_t len){
    int res = 0;
    int sign = 1;
    size_t i = 0;
    if(start[0] == '-'){
        sign = -1;
        i = 1;
    }

    while(i < len && isdigit(start[i])){
        res = res * 10 + (start[i] - '0');
        i++;
    }

    return sign * res;
}

double parseFloat(const char *start, size_t len){
    double res = 0.0;
    double sign = 1.0;
    size_t i = 0;

    if(start[0] == '-'){
        sign = -1.0;
        i = 1;
    }

    while (i < len && isdigit(start[i])) {
        res = res * 10.0 + (start[i] - '0');
        i++;
    }

    if(i < len && start[i] == '.'){
        i++;
        double frac = 0.0;
        double divisor = 10.0;
        while (i < len && isdigit(start[i])) {
            frac += (start[i] - '0') / divisor;
            divisor *= 10.0;
            i++;
        }

        res += frac;
    }

    if (i < len && (start[i] == 'f' || start[i] == 'F')) {
        i++;
    }
    
    return sign * res;
}

int matchLit(const char *start, size_t len, const char *lit) {
    size_t litLen = strlen(lit);
    if (len != litLen) return 0;
    
    return memcmp(start, lit, len) == 0;
}

int bufferEqual(const char *start1, size_t len1, const char *start2, size_t len2) {
    if (len1 != len2) return 0;
    
    return memcmp(start1, start2, len1) == 0;
}