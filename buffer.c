/* buffer.c — YOUR implementation goes here.
 * See server_api.h for the exact signatures and required behaviour.
 */
#include "server_api.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }
    return 0;
}

void buffer_init(conn_buf_t *b) {
    if (b == NULL) {
        return;
    }
    b->len = 0;
    b->line_too_long = 0;
}

int buffer_append(conn_buf_t *b, const char *data, size_t n) {
    if (b == NULL) {
        return -1;
    }

    /* Check if we can fit the data */
    if (b->len + n > CONN_BUF_CAPACITY) {
        /* Would overflow - check if there's a newline in existing + new data */
        /* First, check if there's a newline in existing buffer */
        int has_newline = 0;
        for (size_t i = 0; i < b->len; i++) {
            if (b->data[i] == '\n') {
                has_newline = 1;
                break;
            }
        }
        /* Check in new data too */
        if (!has_newline) {
            for (size_t i = 0; i < n; i++) {
                if (data[i] == '\n') {
                    has_newline = 1;
                    break;
                }
            }
        }

        if (!has_newline) {
            /* No newline found, line is too long */
            b->len = 0;
            b->line_too_long = 1;
            return 0;
        }

        /* Has newline - append as much as we can */
        size_t space = CONN_BUF_CAPACITY - b->len;
        if (n > space) {
            n = space;
        }
    }

    /* Append the data */
    memcpy(b->data + b->len, data, n);
    b->len += n;

    return 0;
}

int buffer_take_line(conn_buf_t *b, char *out, size_t outmax) {
    if (b == NULL || out == NULL) {
        return -1;
    }

    /* If line_too_long is latched, surface it once */
    if (b->line_too_long) {
        out[0] = '\0';
        b->line_too_long = 0;
        return 1;
    }

    /* Find newline in buffer */
    char *newline = memchr(b->data, '\n', b->len);
    if (newline == NULL) {
        return 0;  /* No complete line yet */
    }

    /* Calculate line length (excluding '\n') */
    size_t line_len = newline - b->data;

    /* Check for '\r' before '\n' */
    size_t copy_len = line_len;
    if (copy_len > 0 && b->data[copy_len - 1] == '\r') {
        copy_len--;
    }

    /* Copy to output (truncate if needed) */
    if (copy_len >= outmax) {
        copy_len = outmax - 1;
    }
    memcpy(out, b->data, copy_len);
    out[copy_len] = '\0';

    /* Remove line from buffer (including '\n') */
    size_t consumed = line_len + 1;  /* +1 for '\n' */
    size_t remaining = b->len - consumed;
    if (remaining > 0) {
        memmove(b->data, b->data + consumed, remaining);
    }
    b->len = remaining;

    return 1;
}
