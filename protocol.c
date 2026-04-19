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
    /* TODO: implement */
    (void)line; (void)out;
    return -1;
}

int format_response(const request_t *req, const server_state_t *st,
                    char *out, size_t outlen) {
    /* TODO: implement */
    (void)req; (void)st; (void)out; (void)outlen;
    return -1;
}
