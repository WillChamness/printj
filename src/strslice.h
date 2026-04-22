#ifndef STRING_SLICE_H
#define STRING_SLICE_H

#include <stdbool.h>

#define SLICELEN(slice) (slice.start > slice.stop ? 0 : slice.stop - slice.start)

typedef struct {
    char *borrowed_str;
    unsigned int start;
    unsigned int stop;
} StringSlice;


/*
* returns a slice of the entire string
*/
StringSlice strborrow(char *null_terminated_string);

/*
* returns the first character of the slice
* 
* ***program will halt if input slice is empty***
*/
char strhead(StringSlice slice);

/*
* returns the slice containing all characters 
* except the first
*
* ***program will halt if input slice is empty***
*/
StringSlice strtail(StringSlice slice);

/*
* returns a slice of the first n characters if it 
* contains more than n characters; otherwise, the entire
* slice
*/
StringSlice strtake(unsigned int n, StringSlice slice);

/*
* returns of slice that ignores the first n characters if
* it contains more than n characters; otherwise, an empty
* slice
*/
StringSlice strdrop(unsigned int n, StringSlice slice);

/*
* returns a positive number if s1 is lexicographically greater than s2; otherwise,
* a negative number if s1 is lexicographically less than s2; othewise 0
*/
int strcompare(StringSlice s1, StringSlice s2);

/*
* copies as many characters from the slice to the buffer as possible
*
* the buffer is guaranteed to be terminated by a null character,
* even if the slice length is greater than or equal to the buffer size
*
* does nothing if the buffer size is 0
*/
void strnclone(char *dst, StringSlice src, unsigned int buffer_size);

/*
* returns true if the slice contains the character;
* otherwise, false
*/
bool strcontains(StringSlice slice, char target);

/*
* parses the slice to an integer
*
* ***program halts if the slice is not an formatted
* as an integer***
*/
int parse_int(StringSlice slice);

/*
* parses the slice to a double
*
* ***program halts if the slice is not formatted
* as a double***
*/
double parse_double(StringSlice slice);

/*
* prints all characters in the slice to stdout
*/
void print_slice(StringSlice slice);

/*
* prints all characters in the slice to stdout,
* then prints a newline character
*/
void println_slice(StringSlice slice);

#endif