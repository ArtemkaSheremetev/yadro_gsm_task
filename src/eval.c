#include "eval.h"

#include <stdlib.h>

/* Private declarations */

/* Apply one arithmetic operation to two values */
static int apply_operation(long left, long right, char op, long *result);

/* Get the current numeric value of one formula argument */
static int get_arg_value(const csv_matrix_t *matrix, const arg_t *arg, long *value);

/* Free memory owned by the evaluation stack */
static void destroy_stack(eval_stack_t *stack);

/* Push one evaluation frame onto the explicit DFS stack */
static int push_frame(eval_stack_t *stack,
                      size_t row_index,
                      size_t column_index,
                      int ready_to_compute);

/* Pop one evaluation frame from the explicit DFS stack */
static eval_frame_t pop_frame(eval_stack_t *stack);

/* Push a referenced dependency cell onto the stack if it still needs evaluation */
static int push_dependency(csv_matrix_t *matrix,
                           const arg_t *arg,
                           eval_stack_t *stack);
                           
/* Evaluate one cell and all cells it depends on */
static int evaluate_cell(csv_matrix_t *matrix, size_t row_index, size_t column_index, long *value);

/* Private definitions */
/* Get the current numeric value of one formula argument */
static int get_arg_value(const csv_matrix_t *matrix, const arg_t *arg, long *value) {
    const cell_t *cell;

    if (arg->type == ARG_TYPE_NUMBER) {
        *value = arg->data.number;
        return 1;
    }

    cell = &matrix->rows[arg->data.ref.row.row_index].cells[arg->data.ref.column_index];
    if (cell->type != CELL_TYPE_NUMBER) {
        return 0;
    }

    *value = cell->data.number;
    return 1;
}

/* Free memory owned by the evaluation stack */
static void destroy_stack(eval_stack_t *stack) {
    free(stack->items);
    stack->items = NULL;
    stack->size = 0;
    stack->capacity = 0;
}

/* Push one evaluation frame onto the explicit DFS stack */
static int push_frame(eval_stack_t *stack,
                      size_t row_index,
                      size_t column_index,
                      int ready_to_compute) {
    eval_frame_t *new_stack;
    size_t new_capacity;

    if (stack->size == stack->capacity) {
        new_capacity = (stack->capacity == 0) ? 64 : (stack->capacity * 2);
        new_stack = realloc(stack->items, new_capacity * sizeof(eval_frame_t));
        if (new_stack == NULL) {
            return 0;
        }

        stack->items = new_stack;
        stack->capacity = new_capacity;
    }

    stack->items[stack->size].row_index = row_index;
    stack->items[stack->size].column_index = column_index;
    stack->items[stack->size].ready_to_compute = ready_to_compute;
    stack->size++;
    return 1;
}

/* Pop one evaluation frame from the explicit DFS stack */
static eval_frame_t pop_frame(eval_stack_t *stack) {
    stack->size--;
    return stack->items[stack->size];
}

/* Push a referenced dependency cell onto the stack if it still needs evaluation */
static int push_dependency(csv_matrix_t *matrix,
                           const arg_t *arg,
                           eval_stack_t *stack) {
    cell_t *cell;

    if (arg->type == ARG_TYPE_NUMBER) {
        return 1;
    }

    cell = &matrix->rows[arg->data.ref.row.row_index].cells[arg->data.ref.column_index];

    if (cell->type == CELL_TYPE_EMPTY) {
        cell->state = CELL_STATE_ERROR;
        return 0;
    }

    if (cell->type == CELL_TYPE_NUMBER) {
        return 1;
    }

    if (cell->state == CELL_STATE_ERROR) {
        return 0;
    }

    if (cell->state == CELL_STATE_VISITING) {
        cell->state = CELL_STATE_ERROR;
        return 0;
    }

    return push_frame(stack,
                      arg->data.ref.row.row_index,
                      arg->data.ref.column_index,
                      0);
}

/* Apply one arithmetic operation to two values */
static int apply_operation(long left, long right, char op, long *result) {
    switch (op) {
    case '+':
        *result = left + right;
        return 1;
    case '-':
        *result = left - right;
        return 1;
    case '*':
        *result = left * right;
        return 1;
    case '/':
        if (right == 0) {
            return 0;
        }

        *result = left / right;
        return 1;
    default:
        return 0;
    }
}

/* Evaluate one cell and all cells it depends on */
static int evaluate_cell(csv_matrix_t *matrix, size_t row_index, size_t column_index, long *value) {
    eval_stack_t stack;
    eval_frame_t frame;
    cell_t *cell;
    long left_value;
    long right_value;
    long result;

    cell = &matrix->rows[row_index].cells[column_index];

    if (cell->type == CELL_TYPE_EMPTY) {
        cell->state = CELL_STATE_ERROR;
        return 0;
    }

    if (cell->type == CELL_TYPE_NUMBER) {
        *value = cell->data.number;
        cell->state = CELL_STATE_RESOLVED;
        return 1;
    }

    if (cell->state == CELL_STATE_VISITING) {
        cell->state = CELL_STATE_ERROR;
        return 0;
    }

    if (cell->state == CELL_STATE_ERROR) {
        return 0;
    }

    stack.items = NULL;
    stack.size = 0;
    stack.capacity = 0;

    if (!push_frame(&stack, row_index, column_index, 0)) {
        return 0;
    }

    while (stack.size > 0) {
        frame = pop_frame(&stack);
        cell = &matrix->rows[frame.row_index].cells[frame.column_index];

        if (!frame.ready_to_compute) {
            if (cell->type == CELL_TYPE_EMPTY) {
                cell->state = CELL_STATE_ERROR;
                destroy_stack(&stack);
                return 0;
            }

            if (cell->type == CELL_TYPE_NUMBER) {
                continue;
            }

            if (cell->state == CELL_STATE_ERROR) {
                destroy_stack(&stack);
                return 0;
            }

            if (cell->state == CELL_STATE_VISITING) {
                cell->state = CELL_STATE_ERROR;
                destroy_stack(&stack);
                return 0;
            }

            cell->state = CELL_STATE_VISITING;

            if (!push_frame(&stack,
                            frame.row_index,
                            frame.column_index,
                            1) ||
                !push_dependency(matrix,
                                 &cell->data.formula.right,
                                 &stack) ||
                !push_dependency(matrix,
                                 &cell->data.formula.left,
                                 &stack)) {
                cell->state = CELL_STATE_ERROR;
                destroy_stack(&stack);
                return 0;
            }

            continue;
        }

        if (!get_arg_value(matrix, &cell->data.formula.left, &left_value) ||
            !get_arg_value(matrix, &cell->data.formula.right, &right_value) ||
            !apply_operation(left_value, right_value, cell->data.formula.op, &result)) {
            cell->state = CELL_STATE_ERROR;
            destroy_stack(&stack);
            return 0;
        }

        cell->type = CELL_TYPE_NUMBER;
        cell->state = CELL_STATE_RESOLVED;
        cell->data.number = result;
    }

    destroy_stack(&stack);
    *value = matrix->rows[row_index].cells[column_index].data.number;
    return 1;
}

/* Public definitions */
/* Evaluate all formula cells in the matrix */
int evaluate_matrix(csv_matrix_t *matrix) {
    size_t row_index;
    size_t column_index;
    long value;

    for (row_index = 0; row_index < matrix->rows_size; ++row_index) {
        for (column_index = 0; column_index < matrix->columns_size; ++column_index) {
            if (matrix->rows[row_index].cells[column_index].type != CELL_TYPE_FORMULA) {
                continue;
            }

            if (!evaluate_cell(matrix, row_index, column_index, &value)) {
                return 0;
            }
        }
    }

    return 1;
}
