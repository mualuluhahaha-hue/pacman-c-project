@echo off
REM ---------------------------------------------------------------------
REM run_tests.bat - build everything and run the whole test suite (Windows).
REM MFE113TC group project - text-based Pac-Man (standard C only)
REM Part 4 (Game State & Testing).
REM
REM   1. builds pacman.exe              (gcc *.c -o pacman)
REM   2. builds and runs unit_tests.exe (197 checks, no Python needed)
REM   3. runs the integration tests     (Python runner, or a findstr fallback)
REM ---------------------------------------------------------------------
setlocal
cd /d "%~dp0.."

echo =====================================================
echo  step 1: build the game
echo =====================================================
where gcc >nul 2>nul
if errorlevel 1 (
    echo ERROR: gcc was not found in PATH
    exit /b 1
)
gcc -std=c99 -Wall -Wextra -pedantic -O2 *.c -o pacman.exe
if errorlevel 1 exit /b 1

echo.
echo =====================================================
echo  step 2: build and run the unit tests
echo =====================================================
gcc -std=c99 -Wall -Wextra -pedantic -O2 -I. tests\unit_tests.c rng.c entity.c ^
    map.c game.c ghost.c render.c input.c screen.c -o unit_tests.exe
if errorlevel 1 exit /b 1
unit_tests.exe
if errorlevel 1 (
    echo UNIT TESTS FAILED
    exit /b 1
)

echo.
echo =====================================================
echo  step 3: integration tests
echo =====================================================

REM find a Python interpreter that really works (the Microsoft Store alias
REM "python.exe" exists but cannot execute anything)
set PY=
python -c "import sys" >nul 2>nul
if not errorlevel 1 set PY=python
if not defined PY (
    py -c "import sys" >nul 2>nul
    if not errorlevel 1 set PY=py
)
if not defined PY (
    python3 -c "import sys" >nul 2>nul
    if not errorlevel 1 set PY=python3
)

if defined PY (
    echo using "%PY% tests\run_tests.py"
    %PY% tests\run_tests.py
    if errorlevel 1 (
        echo INTEGRATION TESTS FAILED
        exit /b 1
    )
    echo.
    echo ALL TESTS PASSED
    exit /b 0
)

echo no working Python found - running the findstr based fallback checks
call :expect win      "--map tests\cases\map_win.txt --seed 42 --ghosts 1 --quiet --difficulty 2 --plain" in_win.txt   "RESULT=WIN SCORE=60"
if errorlevel 1 exit /b 1
call :expect die      "--map tests\cases\map_die.txt --seed 42 --ghosts 1 --quiet --difficulty 2 --plain" in_die.txt   "RESULT=LOSE SCORE=0"
if errorlevel 1 exit /b 1
call :expect power    "--map tests\cases\map_power.txt --seed 3 --ghosts 1 --difficulty 2 --plain"        in_power.txt "RESULT=QUIT SCORE=250"
if errorlevel 1 exit /b 1
call :expect bump     "--map tests\cases\map_win.txt --seed 1 --ghosts 1 --quiet --difficulty 2 --plain"  in_bump.txt  "RESULT=WIN SCORE=60 PELLETS_EATEN=6 PELLETS_TOTAL=6 TICKS=8"
if errorlevel 1 exit /b 1
echo ALL FALLBACK CHECKS PASSED
exit /b 0

:expect
REM %1 = case name, %2 = game arguments, %3 = input file, %4 = expected text
pacman.exe %~2 < tests\cases\%3 > tests\logs\%1.txt 2>&1
findstr /C:"%~4" tests\logs\%1.txt >nul
if errorlevel 1 (
    echo   %1 FAIL  ^(expected: %~4^)
    exit /b 1
)
echo   %1 PASS
exit /b 0
