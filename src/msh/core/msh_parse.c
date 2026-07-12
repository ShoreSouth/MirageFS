#include "msh/internal/msh_internal.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

static char *msh_skip_spaces(char *p)
{
    while ((p != NULL) && (*p != 0) && isspace((unsigned char)*p)) {
        p++;
    }

    return p;
}

int msh_parse_line(char *line, msh_argv_t *out)
{
    char *p;
    char quote;

    if ((line == NULL) || (out == NULL)) {
        return 1;
    }

    memset(out, 0, sizeof(*out));
    p = line;
    if (((unsigned char)p[0] == 0xefU) &&
        ((unsigned char)p[1] == 0xbbU) &&
        ((unsigned char)p[2] == 0xbfU)) {
        p += 3;
    }

    while (true) {
        p = msh_skip_spaces(p);
        if ((p == NULL) || (*p == 0) || (*p == '#')) {
            break;
        }

        if (out->argc >= (int)MSH_ARG_MAX) {
            fprintf(stderr, "msh: too many arguments\n");
            return 1;
        }

        if ((*p == '\'') || (*p == '"')) {
            quote = *p;
            p++;
            out->argv[out->argc++] = p;
            while ((*p != 0) && (*p != quote)) {
                p++;
            }
            if (*p != quote) {
                fprintf(stderr, "msh: unclosed quote\n");
                return 1;
            }
            *p++ = 0;
        } else {
            out->argv[out->argc++] = p;
            while ((*p != 0) && !isspace((unsigned char)*p)) {
                p++;
            }
            if (*p == 0) {
                break;
            }
            *p++ = 0;
        }
    }

    return 0;
}
