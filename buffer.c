/* buffer.c — YOUR implementation goes here.
 * See server_api.h for the exact signatures and required behaviour.
 */
#include "server_api.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int set_nonblocking(int fd) {
    /* TODO: implement with fcntl(F_GETFL) then fcntl(F_SETFL, ...|O_NONBLOCK) */
    (void)fd;
    return -1;
}

void buffer_init(conn_buf_t *b) {
    /* TODO: implement */
    (void)b;
}

int buffer_append(conn_buf_t *b, const char *data, size_t n) {
    /* TODO: implement */
    (void)b; (void)data; (void)n;
    return -1;
}

int buffer_take_line(conn_buf_t *b, char *out, size_t outmax) {
    /* TODO: implement */
    (void)b; (void)out; (void)outmax;
    return -1;
}
