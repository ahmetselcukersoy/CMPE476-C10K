/* epollserv.c — YOUR implementation of Part B (epoll event loop).
 *
 * See Section 8 of the project definition.
 * Usage:  ./epollserv [port]   (default port 9091)
 */
#include "server_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : 9091;
    (void)port;
    /* TODO:
     *   - ignore SIGPIPE
     *   - create listening socket, set_nonblocking, register with epoll (EPOLLIN)
     *   - event loop with epoll_wait:
     *       on listen-fd readable: accept-all until EAGAIN, set_nonblocking,
     *         register with EPOLLIN | EPOLLET, attach per-conn struct
     *       on client-fd readable: drain read() until EAGAIN,
     *         buffer_append, then buffer_take_line in a loop,
     *         parse_request / format_response / write response
     *       close on QUIT / peer-close / line_too_long
     */
    fprintf(stderr, "epollserv: not yet implemented\n");
    return 1;
}
