#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "parser.h"
#include "json.h"

void _write_padding(FILE *stream, int nested_level);
void _write_value(JsonValue value, FILE *stream);
void _write_value_formatted(JsonValue value, int nested_level, FILE *stream);
char *_realloc_char_arr(char *old_buffer, unsigned int new_capacity);

EitherErrJson *json_load(char *file) {
    if(file == NULL)
        return NULL;
    FILE *handle = fopen(file, "r");
    if(handle == NULL) {
        fprintf(stderr, "unrecoverable error: '%s' not found or IO error while reading from file\n", file);
        exit(1);
    }
    if(ferror(handle)) {
        fprintf(stderr, "unrecoverable error: '%s' not found or IO error while reading from file'\n", file);
        exit(1);
    }

    unsigned int capacity = 1024;
    unsigned int length = 0;
    char *buffer = malloc(sizeof(char) * capacity);

    char c = fgetc(handle);
    while(c != EOF) {
        if(ferror(handle)) {
            fprintf(stderr, "unrecoverable error: '%s' not found or IO error while reading from file'\n", file);
            exit(1);
        }
        if(length >= capacity) {
            capacity = 2 * length;
            buffer = realloc(buffer, sizeof(char) * capacity);
            if(buffer == NULL) {
                fputs("unrecoverable error: cannot allocate memory to read file\n", stderr);
                exit(1);
            }
        }

        buffer[length] = c;
        c = fgetc(handle);
        length++;
    }
    buffer = realloc(buffer, sizeof(char) * (capacity + 1));
    if(buffer == NULL) {
        fputs("unrecoverable error: cannot allocate memory to read file\n", stderr);
        exit(1);
    }
    buffer[length] = '\0';

    EitherErrJson *either = json_loads(buffer);
    free(buffer);
    fclose(handle);
    return either;
}

EitherErrJson *json_loads(char *json_str) {
    EitherErrJson *either = parse_json(json_str);
    return either;
}

void json_dump(JsonValue value, FILE *stream) {
    if(stream == NULL)
        return;

    _write_value(value, stream);
}

void json_pdump(JsonValue value, FILE *stream) {
    if(stream == NULL)
        return;

    _write_value_formatted(value, 0, stream);
}

void json_println(JsonValue value) {
    _write_value(value, stdout);
    putchar('\n');
}

void json_pprintln(JsonValue value) {
    _write_value_formatted(value, 0, stdout);
    putchar('\n');
}

void _write_padding(FILE *stream, int nested_level) {
    assert(stream != NULL);

    int padding_length = 4 * nested_level;
    for(int i = 0; i < padding_length; i++)
        putc(' ', stream);
}

void _write_value(JsonValue value, FILE *stream) {
    assert(stream != NULL);

    switch(value.type) {
        case JSON_INT:
            fprintf(stream, "%d", value.data.integer);
            break;
        case JSON_DOUBLE:
            fprintf(stream, "%lf", value.data.real);
            break;
        case JSON_STR:
            fprintf(stream, "\"%s\"", value.data.string);
            break;
        case JSON_BOOL:
            fprintf(stream, "%s", value.data.boolean ? "true" : "false");
            break;
        case JSON_NULL:
            fprintf(stream, "null");
            break;
        case JSON_OBJECT:
            {
                JsonObject *obj = value.data.object;
                putc('{', stream);
                for(unsigned int i = 0; i < obj->length; i++) {
                    fprintf(stream, "\"%s\":", obj->keys[i]);
                    _write_value(obj->values[i], stream);
                    if(i < obj->length - 1)
                        putc(',', stream);
                }
                putc('}', stream);
            }
            break;
        case JSON_ARRAY:
            {
                JsonArray *arr = value.data.array;
                putc('[', stream);
                for(unsigned int i = 0; i < arr->length; i++) {
                    _write_value(arr->values[i], stream);
                    if(i < arr->length - 1)
                        putc(',', stream);
                }
                putc(']', stream);
            }
            break;
        default:
            fputs("error while printing JSON value: unknown value type\n", stderr);
            break;
    }
}

void _write_value_formatted(JsonValue value, int nested_level, FILE *stream) {
    assert(stream != NULL);

    switch(value.type) {
        case JSON_INT:
            _write_padding(stream, nested_level);
            fprintf(stream, "%d", value.data.integer);
            break;
        case JSON_DOUBLE:
            _write_padding(stream, nested_level);
            fprintf(stream, "%lf", value.data.real);
            break;
        case JSON_STR:
            _write_padding(stream, nested_level);
            fprintf(stream, "\"%s\"", value.data.string);
            break;
        case JSON_BOOL:
            _write_padding(stream, nested_level);
            fputs(value.data.boolean ? "true" : "false", stream);
            break;
        case JSON_NULL:
            _write_padding(stream, nested_level);
            fputs("null", stream);
            break;
        case JSON_OBJECT:
            {
                _write_padding(stream, nested_level);
                fputs("{\n", stream);
                JsonObject *obj = value.data.object;
                for(unsigned int i = 0; i < obj->length; i++) {
                    _write_padding(stream, nested_level + 1);
                    fprintf(stream, "\"%s\": ", obj->keys[i]);
                    JsonValue next_value = obj->values[i];
                    if(next_value.type == JSON_OBJECT || next_value.type == JSON_ARRAY) {
                        putc('\n', stream);
                        _write_value_formatted(next_value, nested_level + 2, stream);
                    }
                    else
                        _write_value(next_value, stream);

                    if(i < obj->length - 1)
                       fputs(",\n", stream);
                    else
                       putc('\n', stream);
                }
                _write_padding(stream, nested_level);
                putc('}', stream);
            }
            break;
        case JSON_ARRAY:
            {
                _write_padding(stream, nested_level);
                fputs("[\n", stream);
                JsonArray *arr = value.data.array;
                for(unsigned int i = 0; i < arr->length; i++) {
                    JsonValue next_value = arr->values[i];
                    _write_value_formatted(next_value, nested_level + 1, stream);

                    if(i < arr->length - 1)
                        fputs(",\n", stream);
                    else
                        putc('\n', stream);
                }
                _write_padding(stream, nested_level);
                putc(']', stream);
            }
            break;
        default:
            fputs("error while printing JSON value: unknown value type\n", stderr);
    }
}
