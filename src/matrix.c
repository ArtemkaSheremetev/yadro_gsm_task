#include "matrix.h"

#include <stdlib.h>
#include <string.h>

#define COLUMN_MAP_INITIAL_CAPACITY 32
#define ROW_MAP_INITIAL_CAPACITY 32

/* Private declarations */
static void destroy_row(row_t *row);
static inline size_t hash_row_number(long row_number, size_t capacity);
static size_t hash_text(const char *text, size_t length, size_t capacity);

static int column_map_init(column_index_map_t *map, size_t capacity);
static int column_map_insert_no_resize(column_index_map_t *map,
                                       const char *name,
                                       size_t length,
                                       size_t column_index);
static int column_map_rehash(column_index_map_t *map, size_t new_capacity);
static int column_map_insert(column_index_map_t *map,
                             const char *name,
                             size_t length,
                             size_t column_index);
                             
static int row_map_init(row_index_map_t *map, size_t capacity);
static int row_map_insert_no_resize(row_index_map_t *map, long row_number, size_t row_index);
static int row_map_rehash(row_index_map_t *map, size_t new_capacity);
static int row_map_insert(row_index_map_t *map, long row_number, size_t row_index);
static int resolve_arg_reference(const csv_matrix_t *matrix, arg_t *arg);

/* Private definitions */
static void destroy_row(row_t *row) {
    free(row->cells);
    row->cells = NULL;
    row->cells_size = 0;
}

static inline size_t hash_row_number(long row_number, size_t capacity) {
    return (size_t)row_number % capacity;
}

static size_t hash_text(const char *text, size_t length, size_t capacity) {
    size_t i;
    size_t hash;

    hash = 0;

    for (i = 0; i < length; ++i) {
        hash = ((hash << 5) - hash) + (unsigned char)text[i];
    }

    return hash % capacity;
}

static int column_map_init(column_index_map_t *map, size_t capacity) {
    map->entries = calloc(capacity, sizeof(column_index_entry_t));
    if (map->entries == NULL) {
        return 0;
    }

    map->capacity = capacity;
    map->size = 0;
    return 1;
}

static int column_map_insert_no_resize(column_index_map_t *map,
                                       const char *name,
                                       size_t length,
                                       size_t column_index) {
    size_t index;

    index = hash_text(name, length, map->capacity);

    while (map->entries[index].used) {
        if (map->entries[index].length == length &&
            memcmp(map->entries[index].name, name, length) == 0) {
            return 0;
        }

        index = (index + 1) % map->capacity;
    }

    map->entries[index].used = 1;
    map->entries[index].name = name;
    map->entries[index].length = length;
    map->entries[index].column_index = column_index;
    map->size++;
    return 1;
}

static int column_map_rehash(column_index_map_t *map, size_t new_capacity) {
    column_index_entry_t *old_entries;
    size_t old_capacity;
    size_t i;

    old_entries = map->entries;
    old_capacity = map->capacity;

    map->entries = calloc(new_capacity, sizeof(column_index_entry_t));
    if (map->entries == NULL) {
        map->entries = old_entries;
        return 0;
    }

    map->capacity = new_capacity;
    map->size = 0;

    for (i = 0; i < old_capacity; ++i) {
        if (old_entries[i].used &&
            !column_map_insert_no_resize(map,
                                         old_entries[i].name,
                                         old_entries[i].length,
                                         old_entries[i].column_index)) {
            free(old_entries);
            return 0;
        }
    }

    free(old_entries);
    return 1;
}

static int column_map_insert(column_index_map_t *map,
                             const char *name,
                             size_t length,
                             size_t column_index) {
    if (map->capacity == 0 && !column_map_init(map, COLUMN_MAP_INITIAL_CAPACITY)) {
        return 0;
    }

    if ((map->size + 1) * 10 >= map->capacity * 7 &&
        !column_map_rehash(map, map->capacity * 2)) {
        return 0;
    }

    return column_map_insert_no_resize(map, name, length, column_index);
}

static int row_map_init(row_index_map_t *map, size_t capacity) {
    map->entries = calloc(capacity, sizeof(row_index_entry_t));
    if (map->entries == NULL) {
        return 0;
    }

    map->capacity = capacity;
    map->size = 0;
    return 1;
}

static int row_map_insert_no_resize(row_index_map_t *map, long row_number, size_t row_index) {
    size_t index;

    index = hash_row_number(row_number, map->capacity);

    while (map->entries[index].used) {
        if (map->entries[index].row_number == row_number) {
            return 0;
        }

        index = (index + 1) % map->capacity;
    }

    map->entries[index].used = 1;
    map->entries[index].row_number = row_number;
    map->entries[index].row_index = row_index;
    map->size++;
    return 1;
}

static int row_map_rehash(row_index_map_t *map, size_t new_capacity) {
    row_index_entry_t *old_entries;
    size_t old_capacity;
    size_t i;

    old_entries = map->entries;
    old_capacity = map->capacity;

    map->entries = calloc(new_capacity, sizeof(row_index_entry_t));
    if (map->entries == NULL) {
        map->entries = old_entries;
        return 0;
    }

    map->capacity = new_capacity;
    map->size = 0;

    for (i = 0; i < old_capacity; ++i) {
        if (old_entries[i].used &&
            !row_map_insert_no_resize(map,
                                      old_entries[i].row_number,
                                      old_entries[i].row_index)) {
            free(old_entries);
            return 0;
        }
    }

    free(old_entries);
    return 1;
}

static int row_map_insert(row_index_map_t *map, long row_number, size_t row_index) {
    if (map->capacity == 0 && !row_map_init(map, ROW_MAP_INITIAL_CAPACITY)) {
        return 0;
    }

    if ((map->size + 1) * 10 >= map->capacity * 7 &&
        !row_map_rehash(map, map->capacity * 2)) {
        return 0;
    }

    return row_map_insert_no_resize(map, row_number, row_index);
}

static int resolve_arg_reference(const csv_matrix_t *matrix, arg_t *arg) {
    size_t row_index;

    if (arg->type != ARG_TYPE_REF) {
        return 1;
    }

    if (!find_row_index(matrix, arg->data.ref.row.row_number, &row_index)) {
        return 0;
    }

    arg->data.ref.row.row_index = row_index;
    return 1;
}

/* Public definitions */
void destroy_matrix(csv_matrix_t *matrix) {
    size_t i;

    for (i = 0; i < matrix->columns_size; ++i) {
        free(matrix->columns[i].name);
    }

    free(matrix->columns);
    matrix->columns = NULL;
    matrix->columns_size = 0;

    free(matrix->column_map.entries);
    matrix->column_map.entries = NULL;
    matrix->column_map.capacity = 0;
    matrix->column_map.size = 0;

    for (i = 0; i < matrix->rows_size; ++i) {
        destroy_row(&matrix->rows[i]);
    }

    free(matrix->rows);
    matrix->rows = NULL;
    matrix->rows_size = 0;

    free(matrix->row_map.entries);
    matrix->row_map.entries = NULL;
    matrix->row_map.capacity = 0;
    matrix->row_map.size = 0;
}

int build_column_map(csv_matrix_t *matrix) {
    size_t i;

    for (i = 0; i < matrix->columns_size; ++i) {
        if (!column_map_insert(&matrix->column_map,
                               matrix->columns[i].name,
                               strlen(matrix->columns[i].name),
                               i)) {
            return 0;
        }
    }

    return 1;
}

int find_column_index(const csv_matrix_t *matrix,
                      const char *start,
                      size_t length,
                      size_t *column_index) {
    size_t start_index;
    size_t index;

    if (matrix->column_map.capacity == 0) {
        return 0;
    }

    start_index = hash_text(start, length, matrix->column_map.capacity);
    index = start_index;

    while (matrix->column_map.entries[index].used) {
        if (matrix->column_map.entries[index].length == length &&
            memcmp(matrix->column_map.entries[index].name, start, length) == 0) {
            *column_index = matrix->column_map.entries[index].column_index;
            return 1;
        }

        index = (index + 1) % matrix->column_map.capacity;
        if (index == start_index) {
            break;
        }
    }

    return 0;
}

int find_row_index(const csv_matrix_t *matrix, long row_number, size_t *row_index) {
    size_t start;
    size_t index;

    if (matrix->row_map.capacity == 0) {
        return 0;
    }

    start = hash_row_number(row_number, matrix->row_map.capacity);
    index = start;

    while (matrix->row_map.entries[index].used) {
        if (matrix->row_map.entries[index].row_number == row_number) {
            *row_index = matrix->row_map.entries[index].row_index;
            return 1;
        }

        index = (index + 1) % matrix->row_map.capacity;
        if (index == start) {
            break;
        }
    }

    return 0;
}

cell_t *find_cell(csv_matrix_t *matrix, size_t column_index, long row_number) {
    size_t row_index;

    if (!find_row_index(matrix, row_number, &row_index)) {
        return NULL;
    }

    return &matrix->rows[row_index].cells[column_index];
}

int resolve_references(csv_matrix_t *matrix) {
    size_t row_index;
    size_t column_index;
    cell_t *cell;

    for (row_index = 0; row_index < matrix->rows_size; ++row_index) {
        for (column_index = 0; column_index < matrix->columns_size; ++column_index) {
            cell = &matrix->rows[row_index].cells[column_index];

            if (cell->type != CELL_TYPE_FORMULA) {
                continue;
            }

            if (!resolve_arg_reference(matrix, &cell->data.formula.left) ||
                !resolve_arg_reference(matrix, &cell->data.formula.right)) {
                cell->state = CELL_STATE_ERROR;
                return 0;
            }
        }
    }

    return 1;
}

int matrix_append_row(csv_matrix_t *matrix, row_t *row) {
    row_t *new_rows;

    new_rows = realloc(matrix->rows, (matrix->rows_size + 1) * sizeof(row_t));
    if (new_rows == NULL) {
        return 0;
    }

    matrix->rows = new_rows;
    matrix->rows[matrix->rows_size] = *row;
    if (!row_map_insert(&matrix->row_map, row->number, matrix->rows_size)) {
        destroy_row(&matrix->rows[matrix->rows_size]);
        return 0;
    }

    matrix->rows_size++;
    return 1;
}

int print_matrix(FILE *stream, const csv_matrix_t *matrix) {
    size_t row_index;
    size_t column_index;
    const cell_t *cell;

    if (fprintf(stream, ",") < 0) {
        return 0;
    }

    for (column_index = 0; column_index < matrix->columns_size; ++column_index) {
        if (column_index > 0 && fprintf(stream, ",") < 0) {
            return 0;
        }

        if (fprintf(stream, "%s", matrix->columns[column_index].name) < 0) {
            return 0;
        }
    }

    if (fprintf(stream, "\n") < 0) {
        return 0;
    }

    for (row_index = 0; row_index < matrix->rows_size; ++row_index) {
        if (fprintf(stream, "%ld", matrix->rows[row_index].number) < 0) {
            return 0;
        }

        for (column_index = 0; column_index < matrix->columns_size; ++column_index) {
            cell = &matrix->rows[row_index].cells[column_index];

            if (fprintf(stream, ",") < 0) {
                return 0;
            }

            if (cell->type == CELL_TYPE_EMPTY) {
                continue;
            }

            if (cell->type != CELL_TYPE_NUMBER ||
                fprintf(stream, "%ld", cell->data.number) < 0) {
                return 0;
            }
        }

        if (fprintf(stream, "\n") < 0) {
            return 0;
        }
    }

    return 1;
}
