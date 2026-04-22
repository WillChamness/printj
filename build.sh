#!/bin/bash

gcc -O3 -o build/printj src/json.c src/lexer.c src/main.c src/parser.c src/strslice.c
