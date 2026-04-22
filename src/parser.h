#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stdbool.h>

typedef struct JsonObject JsonObject;
typedef struct JsonArray JsonArray;

typedef enum {
    JSON_INT,
    JSON_DOUBLE,
    JSON_STR,
    JSON_BOOL,
    JSON_NULL,
    JSON_OBJECT,
    JSON_ARRAY,
} JsonType;

#define JSON_TYPE_TO_STR(json_type) (       \
    json_type == JSON_INT ? "INTEGER"       \
    : json_type == JSON_DOUBLE ? "DOUBLE"   \
    : json_type == JSON_STR ? "STRING"      \
    : json_type == JSON_BOOL ? "BOOLEAN"    \
    : json_type == JSON_NULL ? "NULL"       \
    : json_type == JSON_OBJECT ? "OBJECT"   \
    : json_type == JSON_ARRAY ? "ARRAY"     \
    : "UNKNOWN JSON TYPE"                   \
)

typedef struct {} JsonNull;

typedef union {
    int integer;
    double real;
    char *string;
    bool boolean;
    JsonNull null;
    JsonObject *object;
    JsonArray *array;
} JsonData;

typedef struct {
    JsonType type;
    JsonData data;
} JsonValue;

typedef struct JsonObject {
    unsigned int length;
    unsigned int capacity;
    char **keys;
    JsonValue *values;
} JsonObject;

typedef struct JsonArray {
    JsonValue *values;
    unsigned int length;
    unsigned int capacity;
} JsonArray;

typedef struct EitherErrJson EitherErrJson;

/*
* returns true if the result is in an error state;
* otherwise false
*/
bool is_error(EitherErrJson *either);

/*
* returns the JSON value if the result is not in an error state;
* otherwise, halts program
* 
* ownership of JsonValue is borrowed - memory will be freed
* upon calling deinit_json()
*/
JsonValue unwrap_json_or_halt(EitherErrJson *either);

/*
* gets the error message if the result is in an error state;
* otherwise, NULL
* 
* ownership of the error message is borrowed
*/
char *parser_error_reason(EitherErrJson *either);

/*
* parses the JSON string
*
* assumes the string is null-terminated
*/
EitherErrJson *parse_json(char *json_str);

/*
* recursively deallocates all memory allocated by the JSON value
* or error
*/
void deinit_json(EitherErrJson *either);

#endif