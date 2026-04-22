#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "strslice.h"
#include "lexer.h"
#include "parser.h"

typedef union {
    char *err;
    JsonValue json;
} ErrorOrJson;

typedef struct EitherErrJson {
    ErrorOrJson result;
    bool ok;
} EitherErrJson;


typedef union {
    char *err;
    JsonObject *obj;
} ErrorOrObj;

typedef struct {
    ErrorOrObj result;
    bool ok;
} EitherErrObj;


typedef union {
    char *err;
    JsonArray *arr;
} ErrorOrArr;

typedef struct {
    ErrorOrArr result;
    bool ok;
} EitherErrArr;


void _deinit_value(JsonValue value);
EitherErrJson _parse(TokenIterator *iterator);
EitherErrObj _parse_object(TokenIterator *iterator);
EitherErrArr _parse_array(TokenIterator *iterator);
char *_create_error_message(TokenIterator *iterator, const char *msg);
void _obj_append(JsonObject *obj, char *key, JsonValue value);
void _arr_append(JsonArray *arr, JsonValue value);
char *_parse_object_key(TokenIterator *iterator);


EitherErrJson *parse_json(char *json_str) {
    TokenIterator *iterator = init_iterator(json_str);
    EitherErrJson either = _parse(iterator);
    EitherErrJson *output = malloc(sizeof(either));
    if(output == NULL) {
        fputs("unrecoverable error: cannot allocate memory for JSON output", stderr);
        exit(1);
    }

    if(!either.ok) {
        output->ok = false;
        output->result.err = either.result.err;
    }
    else if(get_token_type(iterator) != TOKEN_EOF) {
        _deinit_value(either.result.json);
        output->ok = false;
        output->result.err = _create_error_message(iterator, "expected EOF");
    }
    else {
        output->ok = true;
        output->result.json = either.result.json;
    }
    deinit_iterator(iterator);
    return output;
}

bool is_error(EitherErrJson *either) {
    if(either == NULL) {
        fputs("unrecoverable error: cannot get error state of NULL Either\n", stderr);
        exit(1);
    }
    else {
        if(!either->ok)
            assert(either->result.err != NULL);
        return !either->ok;
    }
}

JsonValue unwrap_json_or_halt(EitherErrJson *either) {
    if(either == NULL) {
        fputs("unrecoverable error: cannot unwrap a NULL Either\n", stderr);
        exit(1);
    }
    else if(either->ok) {
        return either->result.json;
    }
    else {
        fputs("unrecoverable error: cannot unwrap Either if it is in an error state\n", stderr);
        exit(1);
    }
}

char *parser_error_reason(EitherErrJson *either) {
    if(either == NULL) {
        fputs("unrecoverable error: cannot get error from NULL Either\n", stderr);
        exit(1);
    }
    else if(either->ok) {
        return NULL;
    }
    else {
        assert(either->result.err != NULL);
        return either->result.err;
    }
}

void deinit_json(EitherErrJson *either) {
    if(either == NULL)
        return;

    if(either->ok) {
        JsonValue value = either->result.json;
        _deinit_value(value);
    }
    else {
        char *err = either->result.err;
        free(err);
    }
    free(either);
}

void _deinit_value(JsonValue value) {
    switch(value.type) {
        case JSON_INT:
        case JSON_DOUBLE:
        case JSON_BOOL:
        case JSON_NULL:
            break;
        case JSON_STR:
            free(value.data.string);
            break;
        case JSON_OBJECT:
            {
                JsonObject *object = value.data.object;
                for(unsigned int i = 0; i < object->length; i++) {
                    free(object->keys[i]);
                    _deinit_value(object->values[i]);
                }
                free(object->keys);
                free(object->values);
                free(object);
            }
            break;
        case JSON_ARRAY:
            {
                JsonArray *array = value.data.array;
                for(unsigned int i = 0; i < array->length; i++) {
                    _deinit_value(array->values[i]);
                }
                free(array->values);
                free(array);
            }
            break;
        default:
            break;
    }
}

EitherErrJson _parse(TokenIterator *iterator) {
    EitherErrJson either;
    if(lexer_error_reason(iterator) != NULL) {
        char *error = lexer_error_reason(iterator);
        char *error_copy = malloc(sizeof(char) * (strlen(error) + 1));
        strcpy(error_copy, error);
        either.ok = false;
        either.result.err = error_copy;
    }
    else {
        TokenType token_type = get_token_type(iterator);
        StringSlice token_value = get_token_value(iterator);
        proceed_next(iterator);

        switch(token_type) {
            case TOKEN_NUMBER:
                either.ok = true;
                if(strcontains(token_value, '.')) {
                    either.result.json.type = JSON_DOUBLE;
                    either.result.json.data = (JsonData){.real = parse_double(token_value)};
                }
                else {
                    either.result.json.type = JSON_INT;
                    either.result.json.data = (JsonData){.integer = parse_int(token_value)};
                }
                break;
            case TOKEN_STRING:
                {
                    unsigned int str_cpy_len = SLICELEN(token_value) + 1;
                    char *str_cpy = malloc(sizeof(char) * str_cpy_len);
                    strnclone(str_cpy, token_value, str_cpy_len);
                    either.ok = true;
                    either.result.json.type = JSON_STR;
                    either.result.json.data = (JsonData){.string = str_cpy};
                }
                break;
            case TOKEN_BOOLEAN:
                either.ok = true;
                either.result.json.type = JSON_BOOL;
                either.result.json.data = (JsonData){.boolean = strcompare(token_value, strborrow("true")) == 0 ? true : false};
                break;
            case TOKEN_NULL:
                either.ok = true;
                either.result.json.type = JSON_NULL;
                either.result.json.data = (JsonData){.null = (JsonNull){}};
                break;
            case TOKEN_LBRACE: 
                {
                    EitherErrObj either_obj = _parse_object(iterator);
                    either.ok = either_obj.ok;
                    if(either_obj.ok) {
                        JsonObject *obj = either_obj.result.obj;
                        either.result.json.type = JSON_OBJECT;
                        either.result.json.data = (JsonData)obj;
                        JsonType type = JSON_OBJECT;
                    }
                    else {
                        char *err = either_obj.result.err;
                        either.result.err = err;
                    }
                }
                break;
            case TOKEN_LBRACKET:
                {
                    EitherErrArr either_arr = _parse_array(iterator);
                    either.ok = either_arr.ok;
                    if(either_arr.ok) {
                        JsonArray *arr = either_arr.result.arr;
                        either.result.json.type = JSON_ARRAY;
                        either.result.json.data = (JsonData)arr;
                    }
                    else {
                        char *err = either_arr.result.err;
                        either.result.err = err;
                    }
                }
                break;
            case TOKEN_EOF:
                either.ok = false;
                either.result.err = _create_error_message(iterator, "unexpected EOF");
                break;
            default:
                {
                    const unsigned int token_copy_length = SLICELEN(token_value) + 1;
                    char token_copy[token_copy_length];
                    strnclone(token_copy, token_value, token_copy_length);
                    char err_msg_buffer[strlen("unexpected token: ") + token_copy_length + 1];
                    strcpy(err_msg_buffer, "unexpected token: ");
                    strcat(err_msg_buffer, token_copy);
                    either.ok = false;
                    either.result.err = _create_error_message(iterator, err_msg_buffer);
                }
                break;
        }
    }

    return either;

}

#define COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator) {                                                       \
    char *lexer_error = lexer_error_reason(iterator);                                                           \
    if(lexer_error != NULL) {                                                                                   \
        either.result.err = malloc(sizeof(char) * (strlen(lexer_error) + 1));                                   \
        if(either.result.err == NULL) {                                                                         \
            fputs("unrecoverable error: could not allocate memory to create error message\n", stderr);          \
                exit(1);                                                                                        \
        }                                                                                                       \
        either.ok = false;                                                                                      \
        strcpy(either.result.err, lexer_error);                                                                 \
        break;                                                                                                  \
    }                                                                                                           \
}

EitherErrObj _parse_object(TokenIterator *iterator) {
    const unsigned int initial_capacity = 256;
    JsonObject *obj = malloc(sizeof(JsonObject));
    char **keys = malloc(sizeof(char *) * initial_capacity);
    JsonValue *values = malloc(sizeof(JsonValue) * initial_capacity);

    if(obj == NULL || keys == NULL || values == NULL) {
        fputs("unrecoverable error: cannot allocate memory to create JSON object\n", stderr);
        exit(1);
    }
    
    obj->length = 0;
    obj->capacity = initial_capacity;
    obj->keys = keys;
    obj->values = values;

    bool done = false;
    EitherErrObj either;
    either.ok = true;

    while(!done) {
        COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator);
        if(get_token_type(iterator) == TOKEN_RBRACE) {
            proceed_next(iterator);
            break;
        }
        COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator);
        char *key = _parse_object_key(iterator);
        if(key == NULL) {
            either.ok = false;
            either.result.err = _create_error_message(iterator, "expected string for object key");
            break;
        }

        COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator);
        if(get_token_type(iterator) != TOKEN_COLON) {
            either.ok = false;
            either.result.err = _create_error_message(iterator, "expected ':' character after object key");
            break;
        }
        proceed_next(iterator);
        EitherErrJson either_json = _parse(iterator);

        if(either_json.ok) {
            JsonValue value = either_json.result.json;
            _obj_append(obj, key, value);
        }
        else {
            either.ok = false;
            either.result.err = either_json.result.err;
            break;
        }
 
        COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator);
        if(get_token_type(iterator) == TOKEN_COMMA) {
            proceed_next(iterator);
            COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator);
            // don't allow {"hello": "world", }
            if(get_token_type(iterator) == TOKEN_RBRACE) {
                either.ok = false;
                either.result.err = _create_error_message(iterator, "unexpected end of JSON object");
                done = true;
            }
        }
        else if(get_token_type(iterator) == TOKEN_RBRACE) { 
            proceed_next(iterator);
            done = true; 
        }
        else if(get_token_type(iterator) == TOKEN_EOF) {
            either.ok = false;
            either.result.err = _create_error_message(iterator, "expected ',' or '}' character for JSON object, got: EOF");
            done = true;
        }
        else {
            StringSlice token_value = get_token_value(iterator);
            const unsigned int token_copy_length = SLICELEN(token_value) + 1;
            char token_copy[token_copy_length];
            strnclone(token_copy, token_value, token_copy_length);
            char base_err_msg[] = "expected ',' or '}' character for JSON object, got: ";
            const unsigned int buffer_size = strlen(base_err_msg) + token_copy_length + 1;
            char err_msg_buffer[buffer_size];
            strcpy(err_msg_buffer, base_err_msg);
            strcat(err_msg_buffer, token_copy);
            either.ok = false;
            either.result.err = _create_error_message(iterator, err_msg_buffer);
            done = true;
        }
    }

    if(either.ok) {
        either.result.obj = obj;
    }
    else{
        assert(either.result.err != NULL);
        for(unsigned int i = 0; i < obj->length; i++) {
            free(obj->keys[i]);
            _deinit_value(obj->values[i]);
        }
        free(obj->keys);
        free(obj);
    }
    return either;
}

void _obj_append(JsonObject *obj, char *key, JsonValue value) {
    assert(obj != NULL);
    assert(key != NULL);

    if(obj->length >= obj->capacity) {
        unsigned int new_capacity = 2 * obj->length;
        obj->keys = realloc(obj->keys, new_capacity * sizeof(char *));
        obj->values = realloc(obj->values, new_capacity * sizeof(JsonValue));
        if(obj->keys == NULL || obj->values == NULL) {
            fputs("unrecoverable error: cannot allocate memeory to create JSON object\n", stderr);
            exit(1);
        }
        obj->capacity = new_capacity;
    }

    obj->keys[obj->length] = key;
    obj->values[obj->length] = value;
    obj->length++;
}

char *_parse_object_key(TokenIterator *iterator) {
    TokenType token_type = get_token_type(iterator);
    StringSlice token_value = get_token_value(iterator);

    if(token_type != TOKEN_STRING)
        return NULL;

    unsigned int buffer_size = SLICELEN(token_value) + 1;
    char *key = malloc(sizeof(char) * buffer_size); 
    if(key == NULL) {
        fputs("unrecoverable error: cannot allocate memeory to create JSON object\n", stderr);
        exit(1);
    }
    strnclone(key, token_value, buffer_size);
    proceed_next(iterator);
    return key;
}

EitherErrArr _parse_array(TokenIterator *iterator) {
    const unsigned int initial_capacity = 256;
    JsonArray *arr = malloc(sizeof(JsonArray));
    JsonValue *values = malloc(sizeof(JsonValue) * initial_capacity);
    if(arr == NULL || values == NULL) {
        fputs("unrecoverable error: cannot allocate memory for JSON array\n", stderr);
        exit(1);
    }
    arr->length = 0;
    arr->capacity = initial_capacity;
    arr->values = values;

    bool done = false;
    EitherErrArr either;
    either.ok = true;

    while(!done) {
        COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator);
        if(get_token_type(iterator) == TOKEN_RBRACKET) {
            proceed_next(iterator);
            break;
        }
        EitherErrJson either_json = _parse(iterator);
        if(either_json.ok) {
            JsonValue json_value = either_json.result.json;
            _arr_append(arr, json_value);
        }
        else {
            char *err = either_json.result.err;
            either.ok = false;
            either.result.err = err;
            break;
        }

        COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator);
        if(get_token_type(iterator) == TOKEN_COMMA) {
            proceed_next(iterator);
            COPY_AND_BREAK_IF_LEXER_ERROR(either, iterator);
            // don't allow [1,2,3,]
            if(get_token_type(iterator) == TOKEN_RBRACKET) {
                char *err = _create_error_message(iterator, "unexpected end of JSON array");
                either.ok = false;
                either.result.err = err;
                done = true;
            }
        }
        else if(get_token_type(iterator) == TOKEN_RBRACKET) {
            proceed_next(iterator);
            done = true;
        }
        else if(get_token_type(iterator) == TOKEN_EOF) {
            either.ok = false;
            either.result.err = _create_error_message(iterator, "expected ',' or ']' character for JSON array, got: EOF");
            done = true;
        }
        else {
            StringSlice token_value = get_token_value(iterator);
            const unsigned int token_copy_length = SLICELEN(token_value) + 1;
            char token_copy[token_copy_length];
            strnclone(token_copy, token_value, token_copy_length);
            char base_err_msg[] = "expected ',' or '}' character for JSON object, got: ";
            const unsigned int buffer_size = strlen(base_err_msg) + token_copy_length + 1;
            char err_msg_buffer[buffer_size];
            strcpy(err_msg_buffer, base_err_msg);
            strcat(err_msg_buffer, token_copy);
            either.ok = false;
            either.result.err = _create_error_message(iterator, err_msg_buffer);
            done = true;
        }
    }

    if(either.ok) {
        either.result.arr = arr;
    }
    else {
        assert(either.result.err != NULL);
        for(unsigned int i = 0; i < arr->length; i++) {
            _deinit_value(arr->values[i]);
        }
        free(arr->values);
        free(arr);
    }

    return either;
}

void _arr_append(JsonArray *arr, JsonValue value) {
    assert(arr != NULL);

    if(arr->length >= arr->capacity) {
        unsigned int new_capacity = 2 * arr->length;
        arr->values = realloc(arr->values, new_capacity * sizeof(JsonValue));
        if(arr->values == NULL) {
            fputs("unrecoverable error: cannot allocate memeory to create JSON object\n", stderr);
            exit(1);
        }
        arr->capacity = new_capacity;
    }

    arr->values[arr->length] = value;
    arr->length++;

}

char *_create_error_message(TokenIterator *iterator, const char *msg) {
    assert(msg != NULL);

    unsigned int row_digit_count = 0;
    unsigned int col_digit_count = 0;

    unsigned int row = get_iterator_row(iterator);
    unsigned int col = get_iterator_col(iterator);

    while(row != 0) {
        row /= 10;
        row_digit_count++;
    }
    while(col != 0) {
        col /= 10;
        col_digit_count++;
    }

    StringSlice token = get_token_value(iterator);

    const unsigned int base_buffer_size = 100; // just in case
    unsigned int buffer_size = base_buffer_size + row_digit_count + col_digit_count + strlen(msg) + 1;
    char *err_buffer = malloc(sizeof(char) * buffer_size);
    if(err_buffer == NULL) {
        fputs("unrecoverable error during paring: could not allocate memory for error message\n", stderr);
        exit(1);
    }

    snprintf(err_buffer, buffer_size, "%d:%d: %s", get_iterator_row(iterator), get_iterator_col(iterator), msg);
    err_buffer[buffer_size - 1] = '\0'; // just in case
    return err_buffer;
}