#ifndef JSON_LEXER_H
#define JSON_LEXER_H

#include "strslice.h"

typedef enum {
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_BOOLEAN,
    TOKEN_NULL,
    TOKEN_EOF,
} TokenType;

#define TOKEN_TYPE_TO_STR(token_type) (                 \
    token_type == TOKEN_LBRACE ? "LEFT BRACE"           \
    : token_type == TOKEN_RBRACE ? "RIGHT BRACE"        \
    : token_type == TOKEN_LBRACKET ? "LEFT BRACKET"     \
    : token_type == TOKEN_RBRACKET ? "RIGHT RBACKET"    \
    : token_type == TOKEN_COLON ? "COLON"               \
    : token_type == TOKEN_COMMA ? "COMMA"               \
    : token_type == TOKEN_NUMBER ? "NUMBER"             \
    : token_type == TOKEN_STRING ? "STRING"             \
    : token_type == TOKEN_BOOLEAN ? "BOOLEAN"           \
    : token_type == TOKEN_NULL ? "NULL"                 \
    : token_type == TOKEN_EOF ? "END OF FILE"           \
    : "UNKNOWN TOKEN")

typedef struct TokenIterator TokenIterator;

/*
* inits the iterator. must be deinitialized later
*
* ownership of the JSON string is borrowed - must be valid until iterator
* is deinitialized
*/
TokenIterator *init_iterator(char *json_str);

/*
* returns NULL if the token is valid; otherwise,
* the reason why the token is invalid
*
* string is guaranteed to be terminated by null character if pointer is not NULL
*
* ownership of the string is borrowed - the iterator
* will free the memory at deinitialization
*/
char *lexer_error_reason(TokenIterator *iterator);

/*
* returns the type of the current token
* 
* ***program halts if the token is invalid***
*/
TokenType get_token_type(TokenIterator *iterator);

/*
* returns a string slice of the next token
* 
* ***program halts if the token is invalid***
*/
StringSlice get_token_value(TokenIterator *iterator);

/*
* gets the row the iterator is currently on
*/
unsigned int get_iterator_row(TokenIterator *iterator);

/*
* gets the column the iterator is currently on
*/
unsigned int get_iterator_col(TokenIterator *iterator);

/*
* destroys the current token and analyzes the next
*
* ***program halts if the current token is invalid***
*/
void proceed_next(TokenIterator *iterator);

/*
* frees the iterator from memory
*/
void deinit_iterator(TokenIterator *iterator);

#endif