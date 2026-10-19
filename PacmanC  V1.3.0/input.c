/*
 * input.c - keyboard input handling (see input.h).
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 2 (Player Control & Rules).
 */
#include <ctype.h>
#include <string.h>
#include "input.h"

#define INPUT_LINE_MAX 128

struct Input input_read(FILE *in)
{
    char line[INPUT_LINE_MAX];
    char *p;
    struct Input result;

    result.kind = INPUT_INVALID;
    result.dr = 0;
    result.dc = 0;
    result.key = '\0';

    if (fgets(line, sizeof line, in) == NULL) {
        result.kind = INPUT_EOF;
        return result;
    }

    /* first non-blank character of the line decides the command */
    p = line;
    while (*p != '\0' && isspace((unsigned char)*p)) {
        p++;
    }
    if (*p == '\0') {
        return result;                      /* empty line: ignored */
    }

    result.key = *p;

    switch (tolower((unsigned char)*p)) {
    case 'w': result.kind = INPUT_MOVE; result.dr = -1; result.dc = 0;  break;
    case 's': result.kind = INPUT_MOVE; result.dr = 1;  result.dc = 0;  break;
    case 'a': result.kind = INPUT_MOVE; result.dr = 0;  result.dc = -1; break;
    case 'd': result.kind = INPUT_MOVE; result.dr = 0;  result.dc = 1;  break;
    case 'q': result.kind = INPUT_QUIT; break;
    default:  result.kind = INPUT_INVALID; break;
    }

    return result;
}
