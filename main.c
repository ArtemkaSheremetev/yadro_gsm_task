#include <stdio.h>

#include "eval.h"
#include "file_utils.h"
#include "matrix.h"
#include "parser.h"

int main(int argc, char *argv[]) {

    FILE *csv_file;
    csv_matrix_t matrix = {0};
    string_t line;
    long file_size;
    size_t initial_capacity;
    int read_status;
    size_t line_number;

    /* Check the command-line arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.csv>\n", argv[0]);
        return 1;
    }
    
    /* Open the CSV file */
    csv_file = fopen(argv[1], "r");
    if (csv_file == NULL) {
        perror("fopen");
        return 1;
    }

    /* Use file size to choose the starting line-buffer size */
    file_size = get_file_size(csv_file);

    /* Stop here if file size cannot be determined */
    if (file_size == -1) {
        fprintf(stderr, "Failed to get size of file: %s\n", argv[1]);
        fclose(csv_file);
        return 1;
    }

    /* Create one reusable line buffer for reading CSV lines */
    initial_capacity = choose_initial_capacity(file_size);
    if (!string_init(&line, initial_capacity)) {
        fprintf(stderr, "Failed to initialize line buffer\n");
        fclose(csv_file); 
        return 1;
    }

    /* Read the header line first */
    read_status = read_line_to_string(csv_file, &line);
    if (read_status < 0) {
        fprintf(stderr, "Failed to read header line\n");
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    if (read_status == 0) {
        fprintf(stderr, "CSV file is empty\n");
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    /* Turn the header into column metadata */
    line_number = 1;
    if (!parse_header_line(&line, &matrix)) {
        fprintf(stderr, "Failed to parse header at line %zu\n", line_number);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    /* Read and parse all data rows */
    while ((read_status = read_line_to_string(csv_file, &line)) > 0) {
        line_number++;

        if (!parse_data_line(&line, &matrix)) {
            fprintf(stderr, "Failed to parse data at line %zu\n", line_number);
            destroy_matrix(&matrix);
            string_destroy(&line);
            fclose(csv_file);
            return 1;
        }
    }

    /* Stop if reading fails somewhere in the middle of the file */
    if (read_status < 0) {
        fprintf(stderr, "Failed to read line %zu\n", line_number + 1);
        destroy_matrix(&matrix);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    /* Resolve formula references after the whole matrix is parsed */
    if (!resolve_references(&matrix)) {
        fprintf(stderr, "Failed to resolve formula references\n");
        destroy_matrix(&matrix);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    /* Evaluate formulas by walking through their dependencies */
    if (!evaluate_matrix(&matrix)) {
        fprintf(stderr, "Failed to evaluate formulas\n");
        destroy_matrix(&matrix);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    /* Print the final CSV table to standard output */
    if (!print_matrix(stdout, &matrix)) {
        fprintf(stderr, "Failed to print matrix\n");
        destroy_matrix(&matrix);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    /* Free everything before exit */
    destroy_matrix(&matrix);
    string_destroy(&line);
    fclose(csv_file);
    return 0;
}
