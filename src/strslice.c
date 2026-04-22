#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "strslice.h"

StringSlice strborrow(char *null_terminated_str) {
    unsigned int len = 0;
    while(null_terminated_str[len] != '\0')
        len++;

    StringSlice result = {
        .borrowed_str = null_terminated_str,
        .start = 0,
        .stop = len,
    };

    return result;
}

char strhead(StringSlice s) {
    if(SLICELEN(s) == 0) {
        fputs("unrecoverable error: cannot take head of empty string\n", stderr);
        exit(1);
    }
    else {
        return s.borrowed_str[s.start];
    }
}

StringSlice strtail(StringSlice slice) {
    if(SLICELEN(slice) == 0) {
        fputs("unrecoverable error: cannot take tail of empty string\n", stderr);
        exit(1);
    }
    else {
        StringSlice result = {
            .borrowed_str = slice.borrowed_str,
            .start = slice.start+1,
            .stop = slice.stop,
        };
        return result;
    }
}

StringSlice strtake(unsigned int n, StringSlice slice) {
    if(n >= SLICELEN(slice))
        return slice;

    StringSlice result = {
        .borrowed_str = slice.borrowed_str,
        .start = slice.start,
        .stop = slice.start + n,
    };
    return result;
}

StringSlice strdrop(unsigned int n, StringSlice slice) {
    StringSlice result;
    if(n >= SLICELEN(slice)) {
        result = (StringSlice){
            .borrowed_str = slice.borrowed_str,
            .start = slice.stop,
            .stop = slice.stop,
        };
    }
    else {
        result = (StringSlice){
            .borrowed_str = slice.borrowed_str,
            .start = slice.start + n,
            .stop = slice.stop,
        };
    }
    return result;
}

int strcompare(StringSlice s1, StringSlice s2) {
    unsigned int s1_len = SLICELEN(s1);
    unsigned int s2_len = SLICELEN(s2);
    for(int i = 0; i < s1_len && i < s2_len; i++) {
        if(s1.borrowed_str[s1.start + i] < s2.borrowed_str[s2.start + i])
            return -1;
        if(s1.borrowed_str[s1.start + i] > s2.borrowed_str[s2.start + i])
            return 1;
    }

    if(s1_len < s2_len)
        return -1;
    else if(s1_len > s2_len)
        return 1;
    else
        return 0;
}

void strnclone(char *dst, StringSlice src, unsigned int buffer_size) {
    if(buffer_size == 0)
        return;

    unsigned int src_len = SLICELEN(src);

    // don't want to overflow the buffer in this case
    if(src_len >= buffer_size) {
        for(unsigned int i = 0; i < buffer_size - 1; i++)
            dst[i] = src.borrowed_str[src.start + i];
        dst[buffer_size - 1] = '\0';
    }
    // buffer contains enough characters, including the null character
    else {
        for(int i = 0; i < src_len; i++)
            dst[i] = src.borrowed_str[src.start + i];
        dst[src_len] = '\0';
    }
}

bool strcontains(StringSlice slice, char target) {
    for(unsigned int i = slice.start; i < slice.stop; i++) {
        if(slice.borrowed_str[i] == target)
            return true;
    }
    return false;
}

int parse_int(StringSlice slice) {
    if(SLICELEN(slice) == 0) {
        fputs("unrecoverable error: tried to parse an empty string to an integer\n", stderr);
        exit(1);
    }
    int sign = strhead(slice) == '-' ? -1 : 1;
    StringSlice num = strhead(slice) == '-' ? strtail(slice) : slice;

    if(SLICELEN(num) == 0) {
        char buffer[SLICELEN(slice) + 1];
        strnclone(buffer, slice, SLICELEN(slice) + 1);
        fprintf(stderr, "unrecoverable error: tried to parse an invalid string to an int: %s\n", buffer);
        exit(1);
    }

    int total = 0;
    int power10 = 1;
    for(unsigned int i = num.stop - 1; i >= num.start; i--) {
        if(num.borrowed_str[i] < '0' || '9' < num.borrowed_str[i]) {
            char buffer[SLICELEN(slice) + 1];
            strnclone(buffer, slice, SLICELEN(slice) + 1);
            fprintf(stderr, "unrecoverable error: tried to parse an invalid string to an int: %s\n", buffer);
            exit(1);
        }
        int digit = num.borrowed_str[i] - '0';
        total += power10 * digit;
        power10 *= 10;
        // prevent integer underflow
        if(i == 0)
            break;
    }

    return sign * total;
}

double parse_double(StringSlice slice) {
    if(SLICELEN(slice) == 0) {
        fputs("unrecoverable error: tried to parse an empty string to a double\n", stderr);
        exit(1);
    }

    int sign = strhead(slice) == '-' ? -1 : 1;
    StringSlice num = strhead(slice) == '-' ? strtail(slice) : slice;

    if(SLICELEN(num) == 0) {
        char buffer[SLICELEN(slice) + 1];
        strnclone(buffer, slice, SLICELEN(slice) + 1);
        fprintf(stderr, "unrecoverable error: tried to parse an invalid string to a double: %s\n", buffer);
        exit(1);
    }

    unsigned int dot_index = num.stop;
    for(unsigned int i = num.start; i < num.stop; i++) {
        if(num.borrowed_str[i] == '.') {
            dot_index = i;
            break;
        }
    }

    StringSlice whole = strtake(dot_index - num.start, num);
    StringSlice fractional = strdrop(dot_index - num.start + 1, num); // don't want to include '.' character

    double total = 0;
    double power10 = 1;
    for(unsigned int i = whole.stop - 1; i >= whole.start; i--) {
        if(whole.borrowed_str[i] < '0' || '9' < whole.borrowed_str[i]) {
            char buffer[SLICELEN(slice) + 1];
            strnclone(buffer, slice, SLICELEN(slice) + 1);
            fprintf(stderr, "unrecoverable error: tried to parse an invalid string to a double: %s\n", buffer);
            exit(1);
        }

        int digit = whole.borrowed_str[i] - '0';
        total += digit * power10;
        power10 *= 10;
        // prevent integer underflow
        if(i == 0)
            break;
    }
    
    power10 = 0.1;
    for(unsigned int i = fractional.start; i < fractional.stop; i++) {
        if(whole.borrowed_str[i] < '0' || '9' < whole.borrowed_str[i]) {
            char buffer[SLICELEN(slice) + 1];
            strnclone(buffer, slice, SLICELEN(slice) + 1);
            fprintf(stderr, "unrecoverable error: tried to parse an invalid string to a double: %s\n", buffer);
            exit(1);
        }
        int digit = whole.borrowed_str[i] - '0';
        total += digit * power10;
        power10 *= 0.1;
    }

    return sign * total;
}

void print_slice(StringSlice slice) {
    if(SLICELEN(slice) == 0)
        return;
    for(unsigned int i = slice.start; i < slice.stop; i++)
        putchar(slice.borrowed_str[i]);
}

void println_slice(StringSlice slice) {
    print_slice(slice);
    putchar('\n');
}