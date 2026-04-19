/* threadserv.c — YOUR implementation of Part A (thread-per-connection).
 *
 * See Section 7 of the project definition.
 * Usage:  ./threadserv [port]   (default port 9090)
 */
#include "server_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>

int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : 9090;
    (void)port;
    /* TODO:
     *   - ignore SIGPIPE
     *   - create AF_INET/SOCK_STREAM socket, setsockopt SO_REUSEADDR
     *   - bind to INADDR_ANY:port, listen
     *   - loop: accept() -> pthread_create(worker) -> pthread_detach
     *   - worker: read lines, parse_request, format_response, write back,
     *             update active_connections under a mutex,
     *             close on QUIT / EOF / line_too_long
     */
    fprintf(stderr, "threadserv: not yet implemented\n");
    return 1;
}
