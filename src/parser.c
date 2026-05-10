#include "parser.h"
#include "matrix.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* Private declarations */
static char *copy_text(const char *src, size_t length);
static int parse_long_token(const char *start, size_t length, long *value);

static int parse_formula_arg(const char *start,
                             size_t length,
                             const csv_matrix_t *matrix,
                             arg_t *arg);

static int init_formula_cell(cell_t *cell,
                             const char *start,
                             size_t length,
                             const csv_matrix_t *matrix);

static int init_cell_from_token(cell_t *cell,
                                const char *start,
                                size_t length,
                                const csv_matrix_t *matrix);

/* Private definitions */
static char *copy_text(const char *src, size_t length)
{
    char *dst;

    dst = malloc(length + 1);
    if (dst == NULL) {
        return NULL;
    }

    memcpy(dst, src, length);
    dst[length] = '\0';
    return dst;
}

static int parse_long_token(const char *start, size_t length, long *value) {
    char *buffer;
    char *endptr;
    size_t i;
    long parsed;

    if (length == 0) {
        return 0;
    }

    buffer = malloc(length + 1);
    if (buffer == NULL) {
        return 0;
    }

    for (i = 0; i < length; ++i) {
        buffer[i] = start[i];
    }
    buffer[length] = '\0';

    errno = 0;
    parsed = strtol(buffer, &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        free(buffer);
        return 0;
    }

    free(buffer);
    *value = parsed;
    return 1;
}

static int parse_formula_arg(const char *start,
                             size_t length,
                             const csv_matrix_t *matrix,
                             arg_t *arg)
{
    size_t split;
    long number;

    if (parse_long_token(start, length, &number)) {
        arg->type = ARG_TYPE_NUMBER;
        arg->data.number = number;
        return 1;
    }

    if (length < 2) {
        return 0;
    }

    split = length;
    while (split > 0 && start[split - 1] >= '0' && start[split - 1] <= '9') {
        split--;
    }

    if (split == 0 || split == length) {
        return 0;
    }

    if (!parse_long_token(start + split, length - split, &number) || number <= 0) {
        return 0;
    }

    arg->type = ARG_TYPE_REF;
    arg->data.ref.row.row_number = number;
    return find_column_index(matrix, start, split, &arg->data.ref.column_index);
}

static int init_formula_cell(cell_t *cell,
                             const char *start,
                             size_t length,
                             const csv_matrix_t *matrix)
{
    size_t op_index;
    char op;

    if (length < 4 || start[0] != '=') {
        return 0;
    }

    op_index = 2;
    while (op_index < length &&
           start[op_index] != '+' &&
           start[op_index] != '-' &&
           start[op_index] != '*' &&
           start[op_index] != '/') {
        op_index++;
    }

    if (op_index >= length - 1) {
        return 0;
    }

    op = start[op_index];
    cell->type = CELL_TYPE_FORMULA;
    cell->state = CELL_STATE_RAW;
    cell->data.formula.op = op;

    if (!parse_formula_arg(start + 1, op_index - 1, matrix, &cell->data.formula.left)) {
        return 0;
    }

    return parse_formula_arg(start + op_index + 1,
                             length - op_index - 1,
                             matrix,
                             &cell->data.formula.right);
}

static int init_cell_from_token(cell_t *cell,
                                const char *start,
                                size_t length,
                                const csv_matrix_t *matrix)
{
    long number;

    if (length == 0) {
        cell->type = CELL_TYPE_EMPTY;
        cell->state = CELL_STATE_RESOLVED;
        cell->data.number = 0;
        return 1;
    }

    if (start[0] == '=') {
        return init_formula_cell(cell, start, length, matrix);
    }

    if (!parse_long_token(start, length, &number)) {
        return 0;
    }

    cell->type = CELL_TYPE_NUMBER;
    cell->state = CELL_STATE_RESOLVED;
    cell->data.number = number;
    return 1;
}

/* Public definitions */
int parse_header_line(const string_t *line, csv_matrix_t *matrix) {
    const char *data;
    size_t start;
    size_t i;
    size_t columns_size;
    column_t *columns;
    size_t column_index;

    data = line->data;

    if (data[0] != ',') {
        return 0;
    }

    start = 1;
    columns_size = 0;

    for (i = 1; i <= line->length; ++i) {
        if (data[i] == ',' || data[i] == '\0') {
            if (i == start) {
                return 0;
            }

            columns_size++;
            start = i + 1;
        }
    }

    columns = calloc(columns_size, sizeof(column_t));
    if (columns == NULL) {
        return 0;
    }

    start = 1;
    column_index = 0;

    for (i = 1; i <= line->length; ++i) {
        if (data[i] == ',' || data[i] == '\0') {
            columns[column_index].name = copy_text(&data[start], i - start);
            if (columns[column_index].name == NULL) {
                while (column_index > 0) {
                    column_index--;
                    free(columns[column_index].name);
                }
                free(columns);
                return 0;
            }

            column_index++;
            start = i + 1;
        }
    }

    matrix->columns = columns;
    matrix->columns_size = columns_size;
    if (!build_column_map(matrix)) {
        while (columns_size > 0) {
            columns_size--;
            free(matrix->columns[columns_size].name);
        }

        free(matrix->columns);
        matrix->columns = NULL;
        matrix->columns_size = 0;
        return 0;
    }

    return 1;
}

int parse_data_line(const string_t *line, csv_matrix_t *matrix) {
    const char *data;
    size_t start;
    size_t i;
    size_t column_index;
    long row_number;
    row_t row;
    size_t row_index;

    data = line->data;

    i = 0;
    while (i < line->length && data[i] != ',') {
        ++i;
    }

    if (i == line->length) {
        return 0;
    }

    if (!parse_long_token(data, i, &row_number) || row_number <= 0) {
        return 0;
    }

    if (find_row_index(matrix, row_number, &row_index)) {
        return 0;
    }

    row.number = row_number;
    row.cells_size = matrix->columns_size;
    row.cells = calloc(matrix->columns_size, sizeof(cell_t));
    if (row.cells == NULL) {
        return 0;
    }

    start = i + 1;
    column_index = 0;

    for (i = start; i <= line->length; ++i) {
        if (data[i] == ',' || data[i] == '\0') {
            if (column_index >= matrix->columns_size) {
                free(row.cells);
                return 0;
            }

            if (!init_cell_from_token(&row.cells[column_index], &data[start], i - start, matrix)) {
                free(row.cells);
                return 0;
            }

            column_index++;
            start = i + 1;
        }
    }

    if (column_index != matrix->columns_size) {
        free(row.cells);
        return 0;
    }

    if (!matrix_append_row(matrix, &row)) {
        free(row.cells);
        return 0;
    }
    return 1;
}
