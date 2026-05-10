#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>

#include "csv_types.h"

/* Public declarations */
void destroy_matrix(csv_matrix_t *matrix);
int build_column_map(csv_matrix_t *matrix);
int find_column_index(const csv_matrix_t *matrix,
                      const char *start,
                      size_t length,
                      size_t *column_index);
int find_row_index(const csv_matrix_t *matrix, long row_number, size_t *row_index);
cell_t *find_cell(csv_matrix_t *matrix, size_t column_index, long row_number);
int resolve_references(csv_matrix_t *matrix);
int matrix_append_row(csv_matrix_t *matrix, row_t *row);
int print_matrix(FILE *stream, const csv_matrix_t *matrix);

#endif
