#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "custom_string.h"
#include "eval.h"
#include "matrix.h"
#include "parser.h"

static void assign_line(string_t *line, const char *text)
{
    size_t i;

    string_clear(line);

    for (i = 0; text[i] != '\0'; ++i) {
        assert(string_append_char(line, text[i]));
    }
}

static void test_valid_pipeline(void)
{
    csv_matrix_t matrix;
    string_t line;
    FILE *fp;
    char buffer[256];
    size_t bytes_read;
    size_t row_index;
    size_t column_index;

    matrix = (csv_matrix_t){0};
    assert(string_init(&line, 32));

    assign_line(&line, ",A,B,Cell");
    assert(parse_header_line(&line, &matrix));

    assign_line(&line, "1,1,0,1");
    assert(parse_data_line(&line, &matrix));
    assign_line(&line, "2,2,=A1+Cell30,0");
    assert(parse_data_line(&line, &matrix));
    assign_line(&line, "30,0,=B1+A1,5");
    assert(parse_data_line(&line, &matrix));

    assert(resolve_references(&matrix));
    assert(evaluate_matrix(&matrix));

    assert(find_column_index(&matrix, "Cell", 4, &column_index));
    assert(column_index == 2);
    assert(find_row_index(&matrix, 30, &row_index));
    assert(matrix.rows[row_index].cells[1].type == CELL_TYPE_NUMBER);
    assert(matrix.rows[row_index].cells[1].data.number == 1);

    fp = tmpfile();
    assert(fp != NULL);
    assert(print_matrix(fp, &matrix));
    rewind(fp);

    bytes_read = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytes_read] = '\0';

    assert(strcmp(buffer,
                  ",A,B,Cell\n"
                  "1,1,0,1\n"
                  "2,2,6,0\n"
                  "30,0,1,5\n") == 0);

    fclose(fp);
    destroy_matrix(&matrix);
    string_destroy(&line);
}

static void test_division_by_zero(void)
{
    csv_matrix_t matrix;
    string_t line;

    matrix = (csv_matrix_t){0};
    assert(string_init(&line, 16));

    assign_line(&line, ",A,B");
    assert(parse_header_line(&line, &matrix));

    assign_line(&line, "1,10,0");
    assert(parse_data_line(&line, &matrix));
    assign_line(&line, "2,=A1/B1,1");
    assert(parse_data_line(&line, &matrix));

    assert(resolve_references(&matrix));
    assert(!evaluate_matrix(&matrix));

    destroy_matrix(&matrix);
    string_destroy(&line);
}

int main(void)
{
    test_valid_pipeline();
    test_division_by_zero();
    return 0;
}
