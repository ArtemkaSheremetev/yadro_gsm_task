#include "eval.h"

/* Private declarations */
static int apply_operation(long left, long right, char op, long *result);
static int evaluate_arg(csv_matrix_t *matrix, const arg_t *arg, long *value);
static int evaluate_cell(csv_matrix_t *matrix, size_t row_index, size_t column_index, long *value);

/* Private definitions */
static int evaluate_arg(csv_matrix_t *matrix, const arg_t *arg, long *value) {
    if (arg->type == ARG_TYPE_NUMBER) {
        *value = arg->data.number;
        return 1;
    }

    return evaluate_cell(matrix,
                         arg->data.ref.row.row_index,
                         arg->data.ref.column_index,
                         value);
}

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

static int evaluate_cell(csv_matrix_t *matrix, size_t row_index, size_t column_index, long *value) {
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

    cell->state = CELL_STATE_VISITING;

    if (!evaluate_arg(matrix, &cell->data.formula.left, &left_value) ||
        !evaluate_arg(matrix, &cell->data.formula.right, &right_value) ||
        !apply_operation(left_value, right_value, cell->data.formula.op, &result)) {
        cell->state = CELL_STATE_ERROR;
        return 0;
    }

    cell->type = CELL_TYPE_NUMBER;
    cell->state = CELL_STATE_RESOLVED;
    cell->data.number = result;
    *value = result;
    return 1;
}

/* Public definitions */
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
