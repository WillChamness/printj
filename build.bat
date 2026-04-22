@echo off

tcc -o build\printj.exe src\json.c src\lexer.c src\main.c src\parser.c src\strslice.c
