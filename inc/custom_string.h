#ifndef CUSTOM_STRING_H
#define CUSTOM_STRING_H

#include <stddef.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} string_t;

/* Public declarations */
int string_init(string_t *str, size_t capacity);
void string_destroy(string_t *str);
void string_clear(string_t *str);

int string_reserve(string_t *str, size_t new_capacity);
int string_append_char(string_t *str, char ch);

#endif
