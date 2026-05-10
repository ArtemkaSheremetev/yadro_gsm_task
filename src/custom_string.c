#include "custom_string.h"

#include <stdlib.h>

/* Public definitions */
int string_init(string_t *str, size_t capacity) {
    if (capacity < 1) {
        capacity = 1;
    }

    str->data = malloc(capacity);
    if (str->data == NULL) {
        str->length = 0;
        str->capacity = 0;
        return 0;
    }

    str->data[0] = '\0';
    str->length = 0;
    str->capacity = capacity;
    return 1;
}

void string_destroy(string_t *str) {
    free(str->data);
    str->data = NULL;
    str->length = 0;
    str->capacity = 0;
}

void string_clear(string_t *str) {
    str->data[0] = '\0';
    str->length = 0;
}

int string_reserve(string_t *str, size_t new_capacity) {
    char *new_data;

    if (new_capacity <= str->capacity) {
        return 1;
    }

    new_data = realloc(str->data, new_capacity);
    if (new_data == NULL) {
        return 0;
    }

    str->data = new_data;
    str->capacity = new_capacity;
    return 1;
}

int string_append_char(string_t *str, char ch) {
    size_t new_capacity;

    if (str->length + 1 >= str->capacity) {
        new_capacity = str->capacity * 2;
        if (new_capacity <= str->capacity) {
            return 0;
        }

        if (!string_reserve(str, new_capacity)) {
            return 0;
        }
    }

    str->data[str->length] = ch;
    str->length++;
    str->data[str->length] = '\0';
    return 1;
}
