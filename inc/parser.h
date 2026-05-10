#ifndef PARSER_H_
#define PARSER_H_

#include "custom_string.h"
#include "csv_types.h"

/* Public declarations */
int parse_header_line(const string_t *line, csv_matrix_t *matrix);
int parse_data_line(const string_t *line, csv_matrix_t *matrix);

#endif
