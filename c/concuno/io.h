#ifndef concuno_io_h
#define concuno_io_h

#include <ctype.h>

#include "core.h"


// TODO Move a lot of this to a separate concuno/string.h of my own?


namespace concuno {


/**
 * Reduce the indent by the canonical amount.
 *
 * TODO Abstract this better into stream facility and for easier auto dedent on
 * TODO error cases.
 */
void cnDedent(cnString* indent);


/**
 * Finds the first delimiter in the string, replaces it with a null char,
 * changes string to point past the placed null char, and returns the address of
 * the original string, if the delimiter is found. If not found, leaves the
 * string unchanged and returns a null pointer. Use like so:
 *
 * char* token = cnDelimit(&text, ',');
 * if (!token) ...
 */
char* cnDelimit(char** string, char delimiter);


/**
 * Finds the first delimiter in the string, replaces it with a null char,
 * changes string to point past the placed null char, puts an integer
 * represention in i, puts the address of the original string in token, and
 * returns true if the delimiter is found and if the token is parseable as an
 * int. If not found, leaves the string unchanged, points string to the end of
 * the original string, token to null, and also returns false. Use like so:
 *
 * if (!cnDelimitInt(&text, &token, &number, ',')) ...
 */
bool cnDelimitInt(char** string, char** token, cnInt* i, char delimiter);


/**
 * Increase the indent by the canonical amount.
 *
 * TODO Abstract this better into stream facility.
 */
bool cnIndent(cnString* indent);


/**
 * Returns the address of the next non-whitespace char in the string, if any.
 * Otherwise returns the address of the null char ending the string.
 */
char* cnNextChar(char* begin);


/**
 * Finds a non-whitespace char if it exists. The end will point past that char
 * if any, or at the already existing null char if at the end. The function
 * returns the value of that non-whitespace char or a null char if none was
 * found. An example follows:
 *
 * char c = cnParseChar(source, &source);
 */
char cnParseChar(char* begin, char** end);


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


/**
 * Returns true if and only if the string ends with ending.
 */
bool cnStrEndsWith(char* string, const char* ending);


}


#endif
