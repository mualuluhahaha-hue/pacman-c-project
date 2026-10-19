@echo off
REM ---------------------------------------------------------------------
REM build.bat - one-command build for Windows (MinGW-w64 / gcc).
REM MFE113TC group project - text-based Pac-Man (standard C only).
REM The command below is the one suggested by the assignment brief.
REM ---------------------------------------------------------------------
setlocal
cd /d "%~dp0"

where gcc >nul 2>nul
if errorlevel 1 (
    echo [build] ERROR: gcc was not found in PATH.
    echo [build] Install MinGW-w64 or run this from a shell that has gcc.
    exit /b 1
)

echo [build] gcc -std=c99 -Wall -Wextra -pedantic -O2 *.c -o pacman.exe
gcc -std=c99 -Wall -Wextra -pedantic -O2 *.c -o pacman.exe
if errorlevel 1 (
    echo [build] FAILED
    exit /b 1
)

echo [build] OK
echo [build] start the game with:  pacman.exe --seed 42 --ghosts 2
endlocal
