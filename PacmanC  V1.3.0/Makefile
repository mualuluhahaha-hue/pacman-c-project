# Makefile - MFE113TC group project: text-based Pac-Man (standard C only)
#
#   make            build the game (same as: gcc *.c -o pacman)
#   make test       build and run the unit tests + the integration tests
#   make clean      remove the binaries
#
# The brief asks for a simple build, so the game target is exactly
# "gcc *.c -o pacman" and every source file of the game lives in this folder.

CC      ?= gcc
CFLAGS  ?= -std=c99 -Wall -Wextra -pedantic -O2

SRC      = main.c game.c map.c entity.c ghost.c render.c input.c rng.c
LIB      = game.c map.c entity.c ghost.c render.c input.c rng.c
HDR      = game.h map.h entity.h ghost.h render.h input.h rng.h

ifeq ($(OS),Windows_NT)
EXE = .exe
RUN = .\
else
EXE =
RUN = ./
endif

all: pacman$(EXE)

pacman$(EXE): $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(SRC) -o pacman$(EXE)

unit_tests$(EXE): tests/unit_tests.c $(LIB) $(HDR)
	$(CC) $(CFLAGS) -I. tests/unit_tests.c $(LIB) -o unit_tests$(EXE)

test: pacman$(EXE) unit_tests$(EXE)
	$(RUN)unit_tests$(EXE)
	python tests/run_tests.py

clean:
	-$(RM) pacman pacman.exe unit_tests unit_tests.exe

.PHONY: all test clean
