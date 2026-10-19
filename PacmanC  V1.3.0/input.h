/*
 * input.h - keyboard input handling.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 2 (Player Control & Rules).
 *
 * Input is line based on purpose: reading single keys without Enter would
 * require conio.h / termios, which are not part of standard C.  The brief
 * only asks for w/a/s/d movement and q to quit, so "type a key, press Enter"
 * is enough and it keeps the game portable to the lab machines.
 */
#ifndef INPUT_H
#define INPUT_H

#include <stdio.h>

enum InputKind {
    INPUT_MOVE = 0,   /* valid movement key                    */
    INPUT_QUIT,       /* q / Q                                 */
    INPUT_EOF,        /* stdin closed (Ctrl+Z / Ctrl+D)        */
    INPUT_INVALID     /* anything else - ignored, no tick lost */
};

struct Input {
    enum InputKind kind;
    int dr, dc;
    char key;
};

/* Read one command.  Never crashes on EOF (requirement 5 of the brief). */
struct Input input_read(FILE *in);

#endif /* INPUT_H */
