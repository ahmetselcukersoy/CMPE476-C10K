/* protocol.c — YOUR implementation goes here.
 * See server_api.h for the exact signatures and required behaviour.
 * See the project definition (CMPE476-C10K-Project.docx, Section 6) for
 * the semantics of each command.
 */
#include "server_api.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <inttypes.h>

int parse_request(const char *line, request_t *out) {
    /* NULL check */
    if (line == NULL || out == NULL) {
        return -1;
    }

    /* Initialize output */
    out->arg[0] = '\0';

    /* Check for exact command matches */
    if (strcmp(line, "PING") == 0) {
        out->kind = CMD_PING;
    } else if (strcmp(line, "TIME") == 0) {
        out->kind = CMD_TIME;
    } else if (strcmp(line, "STATS") == 0) {
        out->kind = CMD_STATS;
    } else if (strcmp(line, "QUIT") == 0) {
        out->kind = CMD_QUIT;
    } else if (strcmp(line, "ECHO") == 0) {
        /* ECHO with no payload */
        out->kind = CMD_ECHO;
    } else if (strncmp(line, "ECHO ", 5) == 0) {
        /* ECHO with payload - exactly one space is separator */
        out->kind = CMD_ECHO;
        strncpy(out->arg, line + 5, MAX_LINE_LEN);
        out->arg[MAX_LINE_LEN] = '\0';
    } else {
        /* Unknown command (includes empty line, leading space, lowercase, etc.) */
        out->kind = CMD_UNKNOWN;
    }

    return 0;
}

int format_response(const request_t *req, const server_state_t *st,
                    char *out, size_t outlen) {
    /* NULL check */
    if (req == NULL || out == NULL || outlen == 0) {
        return -1;
    }

    int n = 0;

    switch (req->kind) {
        case CMD_PING:
            n = snprintf(out, outlen, "PONG");
            break;
        case CMD_ECHO:
            n = snprintf(out, outlen, "%s", req->arg);
            break;
        case CMD_TIME:
            n = snprintf(out, outlen, "%" PRIu64, (uint64_t)time(NULL));
            break;
        case CMD_STATS:
            n = snprintf(out, outlen, "%d", st->active_connections);
            break;
        case CMD_QUIT:
            n = snprintf(out, outlen, "BYE");
            break;
        case CMD_UNKNOWN:
            n = snprintf(out, outlen, "ERR unknown_command");
            break;
        case CMD_TOO_LONG:
            n = snprintf(out, outlen, "ERR line_too_long");
            break;
        default:
            n = snprintf(out, outlen, "ERR unknown_command");
            break;
    }

    /* snprintf returns what would have been written, cap it to actual written */
    if (n < 0) {
        return -1;
    }
    if ((size_t)n >= outlen) {
        n = (int)(outlen - 1);
    }

    return n;
}
