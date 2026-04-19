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

#define MAX_EVENTS 1024

/* Global server state */
static server_state_t g_state = {0, 0};

/* Per-connection context */
typedef struct {
    int fd;
    conn_buf_t buf;
} conn_ctx_t;

/* Close and cleanup a connection */
static void close_connection(int epoll_fd, conn_ctx_t *ctx) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, ctx->fd, NULL);
    close(ctx->fd);
    free(ctx);
    g_state.active_connections--;
}

/* Handle a readable client connection */
static void handle_client(int epoll_fd, conn_ctx_t *ctx) {
    char read_buf[1024];
    char line[MAX_LINE_LEN + 1];
    char response[MAX_RESPONSE_LEN + 1];

    /* Edge-triggered: must drain until EAGAIN */
    while (1) {
        ssize_t n = read(ctx->fd, read_buf, sizeof(read_buf));

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* Done reading for now */
                break;
            }
            /* Real error */
            close_connection(epoll_fd, ctx);
            return;
        }

        if (n == 0) {
            /* Peer closed */
            close_connection(epoll_fd, ctx);
            return;
        }

        /* Append to buffer */
        buffer_append(&ctx->buf, read_buf, (size_t)n);
    }

    /* Process all complete lines */
    int result;
    while ((result = buffer_take_line(&ctx->buf, line, sizeof(line))) == 1) {
        request_t req;

        /* Check if this is the line_too_long signal (empty string after flag cleared) */
        if (line[0] == '\0' && ctx->buf.line_too_long == 0) {
            /* Line was too long - send error and close */
            req.kind = CMD_TOO_LONG;
            req.arg[0] = '\0';
            format_response(&req, &g_state, response, sizeof(response));

            size_t resp_len = strlen(response);
            response[resp_len] = '\n';
            write(ctx->fd, response, resp_len + 1);

            close_connection(epoll_fd, ctx);
            return;
        }

        /* Parse and process request */
        parse_request(line, &req);
        format_response(&req, &g_state, response, sizeof(response));

        /* Write response with newline */
        size_t resp_len = strlen(response);
        response[resp_len] = '\n';
        if (write(ctx->fd, response, resp_len + 1) < 0) {
            close_connection(epoll_fd, ctx);
            return;
        }

        /* Check for QUIT */
        if (req.kind == CMD_QUIT) {
            close_connection(epoll_fd, ctx);
            return;
        }
    }
}

int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : 9091;

    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);

    /* Create listening socket */
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Set SO_REUSEADDR */
    int opt = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listen_fd);
        return 1;
    }

    /* Bind */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    /* Listen */
    if (listen(listen_fd, 128) < 0) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    /* Set listening socket to non-blocking */
    if (set_nonblocking(listen_fd) < 0) {
        perror("set_nonblocking");
        close(listen_fd);
        return 1;
    }

    /* Create epoll instance */
    int epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        close(listen_fd);
        return 1;
    }

    /* Register listening socket with epoll (level-triggered for accept) */
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = NULL;  /* NULL indicates listening socket */
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) < 0) {
        perror("epoll_ctl");
        close(listen_fd);
        close(epoll_fd);
        return 1;
    }

    fprintf(stderr, "epollserv listening on %d\n", port);

    struct epoll_event events[MAX_EVENTS];

    /* Event loop */
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.ptr == NULL) {
                /* Listening socket - accept all pending connections */
                while (1) {
                    int client_fd = accept(listen_fd, NULL, NULL);
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            /* No more pending connections */
                            break;
                        }
                        if (errno == EINTR) {
                            continue;
                        }
                        perror("accept");
                        break;
                    }

                    /* Set non-blocking */
                    if (set_nonblocking(client_fd) < 0) {
                        close(client_fd);
                        continue;
                    }

                    /* Create per-connection context */
                    conn_ctx_t *ctx = malloc(sizeof(conn_ctx_t));
                    if (ctx == NULL) {
                        close(client_fd);
                        continue;
                    }
                    ctx->fd = client_fd;
                    buffer_init(&ctx->buf);

                    /* Register with epoll (edge-triggered) */
                    struct epoll_event client_ev;
                    client_ev.events = EPOLLIN | EPOLLET;
                    client_ev.data.ptr = ctx;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev) < 0) {
                        free(ctx);
                        close(client_fd);
                        continue;
                    }

                    g_state.active_connections++;
                    g_state.total_connections++;
                }
            } else {
                /* Client socket */
                conn_ctx_t *ctx = (conn_ctx_t *)events[i].data.ptr;
                handle_client(epoll_fd, ctx);
            }
        }
    }

    close(listen_fd);
    close(epoll_fd);
    return 0;
}
