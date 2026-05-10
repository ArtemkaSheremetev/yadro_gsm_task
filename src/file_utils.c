#include "file_utils.h"

#define LINE_MAX_SIZE 128

/* Public definitions */
long get_file_size(FILE *fp) {
    long current;
    long size;

    if (fp == NULL) {
        return -1;
    }

    current = ftell(fp);
    if (current == -1) {
        return -1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        return -1;
    }

    size = ftell(fp);
    if (size == -1) {
        return -1;
    }

    if (fseek(fp, current, SEEK_SET) != 0) {
        return -1;
    }

    return size;
}

size_t choose_initial_capacity(long file_size) {
    if (file_size <= 0) {
        return 256;
    }

    if (file_size <= 256) {
        return 256;
    }
    if (file_size <= 512) {
        return 512;
    }
    if (file_size <= 1024) {
        return 1024;
    }
    if (file_size <= 2048) {
        return 2048;
    }
 
    return 4096;
}

int read_line_to_string(FILE *fp, string_t *line) {
    int ch;

    if (fp == NULL || line == NULL) {
        return -1;
    }

    string_clear(line);

    while ((ch = fgetc(fp)) != EOF) {
        if (ch == '\r') {
            ch = fgetc(fp);
            if (ch != '\n' && ch != EOF) {
                if (ungetc(ch, fp) == EOF) {
                    return -1;
                }
            }

            return 1;
        }

        if (ch == '\n') {
            return 1;
        }

        if (line->length >= LINE_MAX_SIZE) {
            return -1;
        }

        if (!string_append_char(line, (char)ch)) {
            return -1;
        }
    }

    if (line->length == 0) {
        return 0;
    }

    return 1;
}
