#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct tcproxy_state {
    int sockIn;
    long interceptIP; /* in network byte order */
}

int tcproxy_init(struct tcproxy_state *state, unsigned short port) {
    state->sockIn = socket(AF_INET, SOCKET_STREAM, 0);
    if (state->sockIn < 0) {
        return -1;
    }
}
