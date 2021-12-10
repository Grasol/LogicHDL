@echo off

gcc lhdlc.c parser.c synthesizer.c instructions.c -Wall -Wextra -march=skylake -o test.exe