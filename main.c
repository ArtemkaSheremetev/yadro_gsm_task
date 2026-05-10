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

    //Bad arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.csv>\n", argv[0]);
        return 1;
    }
    
    //Opening file and checking if fopen succesfull
    csv_file = fopen(argv[1], "r");
    if (csv_file == NULL) {
        perror("fopen");
        return 1;
    }

    file_size = get_file_size(csv_file);

    //Size of file is bad
    if (file_size == -1) {
        fprintf(stderr, "Failed to get size of file: %s\n", argv[1]);
        fclose(csv_file);
        return 1;
    }

    /*Choosing initial capacity for custom string_t for parsing file
      We using file size as metric for start capacity for strings to
      optimize the number of calls of realloc */ 

    initial_capacity = choose_initial_capacity(file_size);
    if (!string_init(&line, initial_capacity)) {
        fprintf(stderr, "Failed to initialize line buffer\n");
        fclose(csv_file);
        return 1;
    }

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

    //Parsing header
    line_number = 1;
    if (!parse_header_line(&line, &matrix)) {
        fprintf(stderr, "Failed to parse header at line %zu\n", line_number);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    //Parsing Data
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

    //Free dinamic memmory if file is corrupted or something goes wrong
    if (read_status < 0) {
        fprintf(stderr, "Failed to read line %zu\n", line_number + 1);
        destroy_matrix(&matrix);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    //Resolving rows indexes: row[index] -> row[number] (using hashmap)
    if (!resolve_references(&matrix)) {
        fprintf(stderr, "Failed to resolve formula references\n");
        destroy_matrix(&matrix);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    //Creating DFS and evaluating formulas
    if (!evaluate_matrix(&matrix)) {
        fprintf(stderr, "Failed to evaluate formulas\n");
        destroy_matrix(&matrix);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    if (!print_matrix(stdout, &matrix)) {
        fprintf(stderr, "Failed to print matrix\n");
        destroy_matrix(&matrix);
        string_destroy(&line);
        fclose(csv_file);
        return 1;
    }

    destroy_matrix(&matrix);
    string_destroy(&line);
    fclose(csv_file);
    return 0;
}
