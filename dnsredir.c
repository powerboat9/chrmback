#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#define DNS_IS_RESPONSE 128
/* opcode is 64, 32, 16, 8 */
#define DNS_IS_AUTHORATIVE 4
#define DNS_IS_TRUNCATED 2
#define DNS_IS_RECURSION_REQUESTED 1
/* high order byte now */
#define DNS_IS_RECURSION_AVAILABLE 32768
/* zero is 16384, 8192, 4096 */
/* responseCode is 2048, 1024, 512, 256 */

struct dns_header {
    unsigned short id;
    unsigned short flags;
    unsigned short questionCount;
    unsigned short answerCount;
    unsigned short athorityCount;
    unsigned short additionalCount;
}

struct dnsredir_state {
    char *blockedDomain;
    int recvSock;
    int err;
    unsigned short port;
}

int dnsredir_init(struct dnsredir_state *state, char *blockedDomain, unsigned short port) {
    srand(time(NULL));
    state->blockedDomain = blockedDomain;
    state->port = port;
    if ((state->recvSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        state->err = state->recvSock;
        return -1;
    }
    struct sockaddr_in me;
    me.sin_family = AF_INET;
    me.sin_addr.s_addr = htons(INADDR_ANY);
    me.sin_port = htons(port);
    if ((state->err = bind(state->recvSock, (struct sockaddr *) &me, sizeof(me))) < 0) {
        close(state->recvSock);
        return -2;
    }
}

int dnsredir_tick(struct dnsredir_state *state) {
    int 
    if (len < 12) {
        return -1;
    }
    memcpy(out, in, 12);
    out->flags = htons(out->flags); /* really means ntohs :P */
    return 0;
}

int dnsredir_tick() {
