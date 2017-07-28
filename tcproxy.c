#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define MAX_CONS 8
#define DEFAULT_RECV_STORE 64
#define MAX_RECV_BUFF 256

#define isRealErr(e) ((e < 0) && (e != EAGAIN) && (e != EWOULDBLOCK))

struct flex_mem {
    char *mem;
    unsigned int size;
}

struct tcproxy_state {
    struct flex_mem urls[MAX_CONS];
    long interceptIP; /* in network byte order */
    int sockIn;
    int err;
    int inSocks[MAX_CONS];
    int outSocks[MAX_CONS];
    char inSockStates[MAX_CONS];
    char cons;
}

int tcpserver_init(struct tcproxy_state *state, long interceptIP, unsigned short port) {
    state->sockIn = socket(AF_INET, SOCKET_STREAM, 0);
    state->interceptIP = interceptIP;
    state->cons = 0;
    state->urls = malloc(sizeof(struct flex_mem) * MAX_CONS);
    if (state->sockIn < 0) {
        state->err = state->sockIn;
        return -1;
    }
    struct sockaddr_in serverAddr;
    zero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);
    if ((state->err = bind(state->sockIn, (struct sockaddr *) &serverAddr, sizeof(serverAddr))) < 1) {
        close(state->sockIn);
        return -2;
    }
    if ((state->err = listen(state->sockIn, MAX_CONS * 2)) < 0) {
        close(state->sockIn);
        return -3;
    }
    int opt = 1;
    if ((state->err = fcntl(state->sockIn, O_NONBLOCK, &opt)) < 0) {
        close(state->sockIn);
        return -4;
    }
    state->err = 0;
}

int tcpserver_tick(struct tcproxy_state *state) {
    if (state->cons < MAX_CONS) {
        state->err = accept(state->sockIn, NULL, NULL);
        if (state->err < 0) {
            if (isRealErr(state->err)) {
                return -1;
            }
        } else {
            state->inSocks[state->cons] = state->err;
            state->outSocks[state->cons] = -1;
            state->urls[state->cons].
            state->cons++;
        }
    }
    char buff[MAX_RECV_BUFF];
    int amountRead;
    for (int i; i < MAX_CONS; i++) {
        amountRead = read(state->inSocks[state->cons - 1], buff, MAX_RECV_BUFF);
        if (amountRead < 0) {
            if (isRealErr(amountRead)) {
                state->err = amountRead;
                return -2;
            }
        } else if (amountRead == 0) {
            if 
