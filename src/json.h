#ifndef JSON_JSON_H
#define JSON_JSON_H
#include <stdio.h>
#include <stdbool.h>
#include "parser.h"

/*
* loads the JSON file into memory and calls json_loads() 
*
* returns NULL if the file string is NULL
*
* ***program halts if file is not found or if an IO error occurs while reading it***
*/
EitherErrJson *json_load(char *file);

/*
* alias for 'parse_json()' in 'parser.h'
*
* returns NULL if the JSON string is NULL
*/
EitherErrJson *json_loads(char *json_str);

/*
* dumps the JSON value to a file handle
*
* does nothing if the file pointer is NULL
*/
void json_dump(JsonValue value, FILE *stream);

/*
* formats the JSON value and dumps the result to a file handle
*
* does nothing if the file pointer is NULL
*/
void json_pdump(JsonValue value, FILE *stream);

/*
* writes the JSON value to stdout
*
* is equivalent to json_dump(value, stdout) with an
* extra new line written to stdout
*/
void json_println(JsonValue value);

/*
* formats the JSON value and writes the result to stdout
*
* is equivalent to json_pdump(value, stdout) with an extra
* new line written to stdout
*/
void json_pprintln(JsonValue value);

#endif