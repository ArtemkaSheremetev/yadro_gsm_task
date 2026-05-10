#ifndef EVAL_H
#define EVAL_H

#include "csv_types.h"

typedef struct {
    size_t row_index;
    size_t column_index;
    int ready_to_compute;
} eval_frame_t;

typedef struct {
    eval_frame_t *items;
    size_t size;
    size_t capacity;
} eval_stack_t;

/* Public declarations */
/* Evaluate all formula cells in the matrix */
int evaluate_matrix(csv_matrix_t *matrix);

#endif
