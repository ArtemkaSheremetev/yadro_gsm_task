#ifndef MATRIX_H
#define MATRIX_H

#include <stdio.h>

#include "csv_types.h"

/* Public declarations */

/* Free all memory owned by the matrix */
void destroy_matrix(csv_matrix_t *matrix);

/* Build a name-to-index map for parsed columns */
int build_column_map(csv_matrix_t *matrix);

/* Find a column index by its name */
int find_column_index(const csv_matrix_t *matrix,
                      const char *start,
                      size_t length,
                      size_t *column_index);

/* Find a row index by its CSV row number */
int find_row_index(const csv_matrix_t *matrix, long row_number, size_t *row_index);

/* Find a cell by column index and CSV row number */
cell_t *find_cell(csv_matrix_t *matrix, size_t column_index, long row_number);

/* Replace row numbers inside formula references with row indexes */
int resolve_references(csv_matrix_t *matrix);

/* Append a parsed row to the matrix and update row_map */
int matrix_append_row(csv_matrix_t *matrix, row_t *row);

/* Print the evaluated matrix in CSV format */
int print_matrix(FILE *stream, const csv_matrix_t *matrix);

#endif
