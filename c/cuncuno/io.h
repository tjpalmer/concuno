#ifndef cuncuno_io_h
#define cuncuno_io_h

#include <stdio.h>

#include "core.h"

/**
 * Clears the string and fills it with the next line from the file. If no data
 * is read, returns 0. Returns the positive count of chars read if no error. If
 * error, returns the negative count of chars read.
 */
cnCount cnReadLine(FILE* file, cnString* string);

#endif
