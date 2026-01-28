#!/bin/bash

# Clean up any potential conflicts in main.c (like _fini)
# We need to perform the same patching on main.c here as we did in Assignment3
sed -i 's/void _fini(void) {}/\/\/ void _fini(void) {}/g' main.c

# Compile test_attack.c with all dependencies
# We include -I. first so it finds the real main.c before mock/main.c
# Then -Imock for the mock USB.h and KeyboardHID.h
# Dependencies: Handlers.c, ReportUtils.c, etc.

gcc -I. -Imock \
    -o test_attack \
    mock/test_attack.c \
    Handlers.c \
    ReportUtils.c \
    IndexToChar.c \
    CharCodeGerman.c \
    -Wno-implicit-function-declaration

if [ $? -eq 0 ]; then
    echo "Compilation successful. Running test..."
    ./test_attack
else
    echo "Compilation failed."
fi
