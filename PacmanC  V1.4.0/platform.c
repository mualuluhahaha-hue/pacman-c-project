/*
 * platform.c - console housekeeping for the colourful display.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 4 (Game State & Testing).
 *
 * On Windows the ANSI escape sequences that screen.c writes are only
 * interpreted after the console has been switched into "virtual terminal"
 * mode, which needs two calls from <windows.h>.  That header is part of the
 * compiler's platform kit (MinGW-w64 ships it), not an external library, and
 * it is used for nothing else: the game logic stays standard C.  Every other
 * platform gets empty stubs, so the code compiles unchanged.
 */
#include <stdio.h>
#include "platform.h"

#ifdef _WIN32
#include <windows.h>

/* MinGW 8 headers predate this flag; the value is fixed by the console API. */
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

void platform_init_console(void)
{
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    {
        CONSOLE_CURSOR_INFO info;

        if (GetConsoleCursorInfo(out, &info)) {
            info.bVisible = FALSE;
            SetConsoleCursorInfo(out, &info);
        }
    }

    SetConsoleTitleA("Pac-Man - MFE113TC Group 6");
#else
    /* POSIX terminals interpret ANSI directly; only hide the cursor. */
    fputs("\x1b[?25l", stdout);
    fflush(stdout);
#endif
}

void platform_restore_console(void)
{
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;

    if (out != INVALID_HANDLE_VALUE && GetConsoleCursorInfo(out, &info)) {
        info.bVisible = TRUE;
        SetConsoleCursorInfo(out, &info);
    }
#else
    fputs("\x1b[?25h", stdout);
    fflush(stdout);
#endif
}
