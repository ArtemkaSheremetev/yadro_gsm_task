#ifndef CUSTOM_STRING_H
#define CUSTOM_STRING_H

#include <stddef.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} string_t;

/* Public declarations */
/* Create a dynamic string with the requested initial capacity */
int string_init(string_t *str, size_t capacity);
/* Free memory owned by the dynamic string */
void string_destroy(string_t *str);
/* Clear the string but keep allocated memory */
void string_clear(string_t *str);

/* Make sure the string has at least the requested capacity */
int string_reserve(string_t *str, size_t new_capacity);
/* Append one character to the end of the string */
int string_append_char(string_t *str, char ch);

#endif
