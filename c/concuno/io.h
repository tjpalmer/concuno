#ifndef concuno_io_h
#define concuno_io_h

#include <ctype.h>

#include "core.h"


/**
 * Finds a non-whitespace string if it exists, overwriting the first trailing
 * whitespace (if any) with a null char. The end will point past that null
 * char if added, or at the already existing null char if at the end. This
 * function returns where the first non-whitespace was found, if any, for usage
 * like below:
 *
 * char* string = cnParseStr(source, &source);
 */
char* cnParseStr(char* begin, char** end);


/**
 * Clears the string and fills it with the next line from the file. If no data
 * is read, returns 0. Returns the positive count of chars read if no error. If
 * error, returns the negative count of chars read.
 */
cnCount cnReadLine(FILE* file, cnString* string);


#endif
