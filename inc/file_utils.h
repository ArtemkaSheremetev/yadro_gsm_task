#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include <stddef.h>
#include <stdio.h>

#include "custom_string.h"

/* Public declarations */
/* Get file size in bytes or return -1 on failure */
long get_file_size(FILE *fp);

/* Choose the starting line-buffer size from file size */
size_t choose_initial_capacity(long file_size);

/* Read one CSV line into string_t without trailing line separators */
int read_line_to_string(FILE *fp, string_t *line);

#endif
