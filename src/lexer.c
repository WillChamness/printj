#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"
#include "strslice.h"

#ifdef _WIN32
    #define snprintf _snprintf
#endif

typedef struct {
    TokenType type;
    StringSlice value;
} Token;

typedef struct TokenIterator {
    StringSlice json_str;
    unsigned int row;
    unsigned int col;
    Token current_token;
    char *error_reason;
} TokenIterator;


void _analyze_next(TokenIterator *iterator);
unsigned int _analyze_number(TokenIterator *iterator);
unsigned int _analyze_string(TokenIterator *iterator);
unsigned int _analyze_keyword(TokenIterator *iterator);
void _attach_error_message(TokenIterator *iterator, const char *msg);


TokenIterator *init_iterator(char *json_str) {
    if(json_str == NULL) {
        fputs("unrecoverable error during lexical analysis: cannot iterate over NULL JSON string\n", stderr);
        exit(1);
    }
    TokenIterator *iterator = malloc(sizeof(TokenIterator));
    if(iterator == NULL) {
        fputs("unrecoverable error during lexical analysis: cannot allocate memory to iterate over JSON string\n", stderr);
        exit(1);
    }

    iterator->json_str = strborrow(json_str);
    iterator->row = 1;
    iterator->col = 1;
    iterator->error_reason = NULL;
    _analyze_next(iterator);
    return iterator;
}

char *lexer_error_reason(TokenIterator *iterator) {
    return iterator->error_reason;
}

TokenType get_token_type(TokenIterator *iterator) {
    if(iterator->error_reason != NULL) {
        fprintf(stderr, "unrecoverable error: cannot get type of invalid token\n"
        "error reason: %s\n", iterator->error_reason);
        exit(1);
    }
    Token token = iterator->current_token;
    return token.type;
}

StringSlice get_token_value(TokenIterator *iterator) {
    if(iterator->error_reason != NULL) {
        fputs("unrecoverable error: cannot get value of invalid token\n", stderr);
        exit(1);
    }
    Token token = iterator->current_token;
    return token.value;
}

unsigned int get_iterator_row(TokenIterator *iterator) {
    if(iterator == NULL) {
        fputs("unrecoverable error: cannot get row from NULL iterator\n", stderr);
        exit(1);
    }
    return iterator->row;
}

unsigned int get_iterator_col(TokenIterator *iterator) {
    if(iterator == NULL) {
        fputs("unrecoverable error: cannot get column from NULL iterator\n", stderr);
        exit(1);
    }
    return iterator->col;
}

void proceed_next(TokenIterator *iterator) {
   if(iterator->error_reason != NULL) {
    fputs("unrecoverbale error during lexical analysis: cannot proceed to next token if current token is invalid", stderr);
    exit(1);
   } 
   _analyze_next(iterator);
}

void deinit_iterator(TokenIterator *iterator) {
    if(iterator != NULL) {
        if(iterator->error_reason != NULL)
            free(iterator->error_reason);
        free(iterator);
    }
}


/*
* attaches the next token to the iterator if there are no errors;
* otherwise, attaches the error message
*/
void _analyze_next(TokenIterator *iterator) {
    if(SLICELEN(iterator->json_str) == 0) {
        iterator->current_token.type = TOKEN_EOF;
        iterator->current_token.value = iterator->json_str;
        return;
    }

    TokenType token_type;
    StringSlice token_value;

    switch(strhead(iterator->json_str)) {
        case '{':
            token_type = TOKEN_LBRACE;
            token_value = strtake(1, iterator->json_str);
            iterator->json_str = strdrop(1, iterator->json_str);
            iterator->col++;
            break;
        case '}':
            token_type = TOKEN_RBRACE;
            token_value = strtake(1, iterator->json_str);
            iterator->json_str = strdrop(1, iterator->json_str);
            iterator->col++;
            break;
        case '[':
            token_type = TOKEN_LBRACKET;
            token_value = strtake(1, iterator->json_str);
            iterator->json_str = strdrop(1, iterator->json_str);
            iterator->col++;
            break;
        case ']':
            token_type = TOKEN_RBRACKET;
            token_value = strtake(1, iterator->json_str);
            iterator->json_str = strdrop(1, iterator->json_str);
            iterator->col++;
            break;
        case ':':
            token_type = TOKEN_COLON;
            token_value = strtake(1, iterator->json_str);
            iterator->json_str = strdrop(1, iterator->json_str);
            iterator->col++;
            break;
        case ',':
            token_type = TOKEN_COMMA;
            token_value = strtake(1, iterator->json_str);
            iterator->json_str = strdrop(1, iterator->json_str);
            iterator->col++;
            break;
        case ' ':
        case '\t':
        case '\r':
            iterator->col++;
            iterator->json_str = strtail(iterator->json_str);
            _analyze_next(iterator);
            return;
        case '\n':
            iterator->col = 1;
            iterator->row++;
            iterator->json_str = strtail(iterator->json_str);
            _analyze_next(iterator);
            return;
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            {
                unsigned int token_length = _analyze_number(iterator);
                if(token_length == 0) {
                    assert(iterator->error_reason != NULL);
                    return;
                }
                else {
                    token_type = TOKEN_NUMBER;
                    token_value = strtake(token_length, iterator->json_str);
                    iterator->json_str = strdrop(token_length, iterator->json_str);
                    iterator->col += token_length;
                }
            }
            break;
        case '"':
            {
                unsigned int token_length = _analyze_string(iterator);
                if(iterator->error_reason == NULL) {
                    token_type = TOKEN_STRING;
                    token_value = strtake(token_length, strtail(iterator->json_str)); // don't want to include opening '"' character
                    iterator->json_str = strdrop(token_length + 2, iterator->json_str); // dont' want to include closing '"' character
                    iterator->col += token_length + 2;
                }
                else
                    {return;}
            }
            break;
        default:
            {
                unsigned int keyword_length = _analyze_keyword(iterator);
                if(keyword_length == 0) {
                    assert(iterator->error_reason != NULL);
                    return;
                }
                token_value = strtake(keyword_length, iterator->json_str);
                iterator->json_str = strdrop(keyword_length, iterator->json_str);
                if(strcompare(token_value, strborrow("true")) == 0) {
                    token_type = TOKEN_BOOLEAN;
                }
                else if(strcompare(token_value, strborrow("false")) == 0) {
                    token_type = TOKEN_BOOLEAN;
                }
                else if(strcompare(token_value, strborrow("null")) == 0) {
                    token_type = TOKEN_NULL;
                }
                else {
                    _attach_error_message(iterator, "unexpected token");
                    return;
                }
            }
    }

    iterator->current_token.type = token_type;
    iterator->current_token.value = token_value;
}

/*
* returns the number of characters representing the number if there
* are no errors; otherwise, 0
* 
* attaches an error message to the iterator if there is an error
*/
unsigned int _analyze_number(TokenIterator *iterator) {
    if(SLICELEN(iterator->json_str) == 0) {
        _attach_error_message(iterator, "cannot convert empty JSON string to number token");
        return 0;
    }    

    unsigned int result = 0;
    StringSlice current = iterator->json_str;
    bool is_negative;
    if(strhead(current) == '-') {
        is_negative = true;
        current = strtail(current);
        if(SLICELEN(current) == 0) {
            _attach_error_message(iterator, "unexpected character: '-'");
            return 0;
        }
    }
    else { is_negative = false; }

    bool seen_leading_zero = false;
    bool seen_dot = false;
    bool reached_end_of_number = false;

    while(SLICELEN(current) != 0 && !reached_end_of_number) {
        char current_head = strhead(current);
        if(current_head == '0' && result == 0) {
            seen_leading_zero = true;
        }
        else if('0' <= current_head && current_head <= '9') {
            // don't allow "0123"
            if(seen_leading_zero) {
                _attach_error_message(iterator, "number contains leading zero");
                return 0;
            }
        }
        else if(current_head == '.') {
            // don't allow "123.456.789"
            if(seen_dot) {
                _attach_error_message(iterator, "number contains multiple '.' characters");
                return 0;
            }
            else { seen_dot = true; }
        }
        else {
            // need to distinguish between "123true" and "123 true"
            if(('a' <= current_head && current_head <= 'z')
                || ('A' <= current_head && current_head <= 'Z')
                || current_head == '"') {
                _attach_error_message(iterator, "unexpected token");
                return 0;
            }
            else { reached_end_of_number = true; }
        }
        if(!reached_end_of_number) {
            result++;
            current = strtail(current);
        }
    }

    return is_negative ? result + 1 : result;
}

/*
* if there are no errors, returns the number of characters between 
* the opening and closing quote characters, not including either 
* quote characters; otherwise, returns 0
* 
* attaches an error message to the iterator if there is an error
*/
unsigned int _analyze_string(TokenIterator *iterator) {
    assert(strhead(iterator->json_str) == '"');

    StringSlice current = strtail(iterator->json_str);
    unsigned int result = 0;

    while(SLICELEN(current) > 0 && strhead(current) != '"') {
        // case where string contains escaped character
        if(strhead(current) == '\\') {
            current = strtail(current);
            result++;
            if(SLICELEN(current) > 0) {
                current = strtail(current);
                result++;
            }
        }
        else {
            current = strtail(current);
            result++;
        }
    }

    if(SLICELEN(current) == 0) {
        _attach_error_message(iterator, "string literal not terminated; EOF reached");
        return 0;
    }
    else {
        return result;
    }
}

/*
* returns the number of alphanumberic characters representing the keyword
* if there are no errors; otherwise, 0
*
* attaches an error message to the iterator if there is an error
*/
unsigned int _analyze_keyword(TokenIterator *iterator) {
    assert(SLICELEN(iterator->json_str) > 0);
    unsigned int result = 0;
    StringSlice current = iterator->json_str;
    char current_char = strhead(current);

    while(SLICELEN(current) > 0 && (
        ('a' <= current_char && current_char <= 'z')
        || ('A' <= current_char && current_char <= 'Z')
        || ('0' <= current_char && current_char <= '9')
    )) {
        result++;
        current = strtail(current);
        if(SLICELEN(current) > 0)
            current_char = strhead(current);
    }

    assert(result > 0);
    return result;
}

/*
* attaches an error to the iterator
*/
void _attach_error_message(TokenIterator *iterator, const char *msg) {
    if(msg == NULL)
        return;

    unsigned int row_digit_count = 0;
    unsigned int col_digit_count = 0;

    unsigned int row = iterator->row;
    while(row != 0) {
        row /= 10;
        row_digit_count++;
    }
    
    unsigned int col = iterator->col;
    while(col != 0) {
        col /= 10;
        col_digit_count++;
    }

    const unsigned int base_buffer_size = 100; // just in case
    unsigned int buffer_size = base_buffer_size + row_digit_count + col_digit_count + strlen(msg) + 1;
    char *error_buffer = malloc(sizeof(char) * buffer_size);
    if(error_buffer == NULL) {
        fputs("unrecoverable error during JSON lexical analysis: could not allocate memory for error message\n", stderr);
        exit(1);
    }

    snprintf(error_buffer, buffer_size, "%d:%d: %s", iterator->row, iterator->col, msg);
    error_buffer[buffer_size - 1] = '\0'; // just in case
    iterator->error_reason = error_buffer;
}
