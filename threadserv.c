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

/* Global server state */
static server_state_t g_state = {0, 0};
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Worker thread argument */
typedef struct {
    int client_fd;
} worker_arg_t;

/* Worker thread function */
static void *worker_thread(void *arg) {
    worker_arg_t *warg = (worker_arg_t *)arg;
    int client_fd = warg->client_fd;
    free(warg);

    /* Increment active connections */
    pthread_mutex_lock(&g_mutex);
    g_state.active_connections++;
    g_state.total_connections++;
    pthread_mutex_unlock(&g_mutex);

    /* Initialize per-connection buffer */
    conn_buf_t buf;
    buffer_init(&buf);

    char read_buf[1024];
    char line[MAX_LINE_LEN + 1];
    char response[MAX_RESPONSE_LEN + 1];

    int should_close = 0;

    while (!should_close) {
        /* Read from socket */
        ssize_t n = read(client_fd, read_buf, sizeof(read_buf));

        if (n <= 0) {
            /* EOF or error */
            break;
        }

        /* Append to buffer */
        buffer_append(&buf, read_buf, (size_t)n);

        /* Process all complete lines */
        int result;
        while ((result = buffer_take_line(&buf, line, sizeof(line))) == 1) {
            request_t req;

            /* Check if this is the line_too_long signal */
            if (line[0] == '\0' && buf.line_too_long == 0) {
                /* This was a line_too_long error surfaced */
                req.kind = CMD_TOO_LONG;
                req.arg[0] = '\0';
                format_response(&req, &g_state, response, sizeof(response));

                /* Write response with newline */
                size_t resp_len = strlen(response);
                response[resp_len] = '\n';
                if (write(client_fd, response, resp_len + 1) < 0) {
                    should_close = 1;
                    break;
                }
                should_close = 1;
                break;
            }

            /* Parse and process request */
            parse_request(line, &req);
            format_response(&req, &g_state, response, sizeof(response));

            /* Write response with newline */
            size_t resp_len = strlen(response);
            response[resp_len] = '\n';
            if (write(client_fd, response, resp_len + 1) < 0) {
                should_close = 1;
                break;
            }

            /* Check for QUIT */
            if (req.kind == CMD_QUIT) {
                should_close = 1;
                break;
            }
        }
    }

    /* Close socket */
    close(client_fd);

    /* Decrement active connections */
    pthread_mutex_lock(&g_mutex);
    g_state.active_connections--;
    pthread_mutex_unlock(&g_mutex);

    return NULL;
}

int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : 9090;

    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);

    /* Create socket */
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

    fprintf(stderr, "threadserv listening on %d\n", port);

    /* Accept loop */
    while (1) {
        int client_fd = accept(listen_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("accept");
            continue;
        }

        /* Create worker argument */
        worker_arg_t *warg = malloc(sizeof(worker_arg_t));
        if (warg == NULL) {
            close(client_fd);
            continue;
        }
        warg->client_fd = client_fd;

        /* Create and detach worker thread */
        pthread_t tid;
        if (pthread_create(&tid, NULL, worker_thread, warg) != 0) {
            free(warg);
            close(client_fd);
            continue;
        }
        pthread_detach(tid);
    }

    close(listen_fd);
    return 0;
}
