/*
 * platform.h - the only non-portable code in the project.
 *
 * MFE113TC Group Project - text-based Pac-Man (standard C only)
 * Part 4 (Game State & Testing).
 *
 * Everything here is console housekeeping for the colourful display:
 * enabling ANSI escape processing on Windows, hiding the text cursor and
 * setting the window title.  The game itself never calls an operating system
 * function; on Linux and macOS every function is an empty stub, so the project
 * still builds with plain standard C everywhere.
 *
 * --plain never calls any of it.
 */
#ifndef PLATFORM_H
#define PLATFORM_H

/* Called once before the title screen is drawn. */
void platform_init_console(void);

/* Called before the program ends: restores the cursor and the colours. */
void platform_restore_console(void);

#endif /* PLATFORM_H */
