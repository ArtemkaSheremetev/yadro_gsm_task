#ifndef CSV_TYPES_H
#define CSV_TYPES_H

#include <stddef.h>

typedef enum {
    CELL_TYPE_EMPTY = 0,
    CELL_TYPE_NUMBER,
    CELL_TYPE_FORMULA
} cell_type_t;

typedef enum {
    CELL_STATE_RAW = 0,
    CELL_STATE_VISITING,
    CELL_STATE_RESOLVED,
    CELL_STATE_ERROR
} cell_state_t;

typedef enum {
    ARG_TYPE_NUMBER = 0,
    ARG_TYPE_REF
} arg_type_t;

typedef struct {
    arg_type_t type;

    union {
        long number;
        struct {
            size_t column_index;
            union {
                long row_number;
                size_t row_index;
            } row;
        } ref;
    } data;
} arg_t;

typedef struct {
    cell_type_t type;
    cell_state_t state;

    union {
        long number;
        struct {
            char op;
            arg_t left;
            arg_t right;
        } formula;
    } data;
} cell_t;

typedef struct {
    long number;
    cell_t *cells;
    size_t cells_size;
} row_t;

typedef struct {
    char *name;
} column_t;

typedef struct {
    int used;
    const char *name;
    size_t length;
    size_t column_index;
} column_index_entry_t;

typedef struct {
    column_index_entry_t *entries;
    size_t capacity;
    size_t size;
} column_index_map_t;

typedef struct {
    int used;
    long row_number;
    size_t row_index;
} row_index_entry_t;

typedef struct {
    row_index_entry_t *entries;
    size_t capacity;
    size_t size;
} row_index_map_t;

typedef struct {
    column_t *columns;
    size_t columns_size;
    column_index_map_t column_map;
    row_t *rows;
    size_t rows_size;
    row_index_map_t row_map;
} csv_matrix_t;

#endif
