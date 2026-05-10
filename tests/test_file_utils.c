#include <assert.h>
#include <stdio.h>

#include "file_utils.h"

static void write_text(FILE *fp, const char *text)
{
    while (*text != '\0') {
        fputc(*text, fp);
        text++;
    }

    rewind(fp);
}

int main(void)
{
    FILE *fp;
    string_t line;
    int status;
    int i;

    assert(choose_initial_capacity(-1) == 256);
    assert(choose_initial_capacity(16) == 256);

    assert(string_init(&line, 8));

    fp = tmpfile();
    assert(fp != NULL);
    write_text(fp, "alpha\r\nbeta\n");

    status = read_line_to_string(fp, &line);
    assert(status == 1);
    assert(line.length == 5);
    assert(line.data[0] == 'a');
    assert(line.data[4] == 'a');

    status = read_line_to_string(fp, &line);
    assert(status == 1);
    assert(line.length == 4);
    assert(line.data[0] == 'b');
    assert(line.data[3] == 'a');

    status = read_line_to_string(fp, &line);
    assert(status == 0);
    fclose(fp);

    fp = tmpfile();
    assert(fp != NULL);
    for (i = 0; i < 129; ++i) {
        fputc('x', fp);
    }
    rewind(fp);

    status = read_line_to_string(fp, &line);
    assert(status == -1);
    fclose(fp);

    string_destroy(&line);
    return 0;
}
