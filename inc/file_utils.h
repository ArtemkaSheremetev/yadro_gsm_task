#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stddef.h>
#include <stdio.h>

#include "custom_string.h"

/* Public declarations */
long get_file_size(FILE *fp);
size_t choose_initial_capacity(long file_size);
int read_line_to_string(FILE *fp, string_t *line);

#endif
