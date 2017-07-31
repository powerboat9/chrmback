#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#define MAX_CONS 8
#define DEFAULT_RECV_STORE 64
#define MAX_RECV_BUFF 256

#define isRealErr(e) ((e < 0) && (e != EAGAIN) && (e != EWOULDBLOCK))

struct flex_mem {
    char *mem;
    unsigned int pos;
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

char headerMatch[] = "CONNECT _ HTTP/1.1\n";
#define STATE_URL 8
#define STATE_HEADERS 19
#define STATE_HEADERS_FREE 20
#define STATE_PROXY 21

char nopeNopeNope[] = "HTTP/1.1 400 Bad Request\nServer: tcproxy\nContent-Length: 0\n\n"

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

void cleanup(struct tcproxy *state, int sock) {
    

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
            state->inSockStates[state->cons] = 0;
            state->cons++;
        }
    }
    char buff[MAX_RECV_BUFF];
    char *dtaP;
    int amountRead;
    for (int i = 0; i < state->cons; i++) {
        amountRead = read(state->inSocks[i], buff, MAX_RECV_BUFF);
        if (amountRead < 0) {
            if (isRealErr(amountRead)) {
                state->err = amountRead;
                return -2;
            }
        } else if (amountRead == 0) {
            if ((state->inSockStates[i] >= STATE_URL) && (state->inSockStates[i] < STATE_PROXY)) {
#ifdef ZERO_SECURE
                zero(state->urls[i].mem, state->urls[i].size);
#endif
                free(state->urls[i].mem);
            }
            if (state->inSockStates[i] == STATE_PROXY) {
                close(state->outSocks[i]);
            }
            close(state->inSocks[i]);
            memmove(state->inSocks + i, state->inSocks + i + 1, (state->cons - i) * 2);
            memmove(state->outSocks + i, state->outSocks + i + 1, (state->cons - i) * 2);
            memmove(state->inSockStates + i, state->inSockStates + i + 1, state->cons - i);
            memmove(state->urls + i, state->urls + i + 1, (state->cons - i) * sizeof(struct flex_mem));
            state->cons--;
#ifdef ZERO_SECURE
            state->inSocks[state->cons] = 0;
            state->outSocks[state->cons] = 0;
            state->inSockStates[state->cons] = 0;
            zero(state->urls + state->cons, sizeof(struct flex_mem));
#endif
        } else {
            dtaP = buff;
            if ((state->inSockStates[i] < STATE_HEADERS) && (state->inSockStates[i] < STATE_URL)) {
                for (int j = state->inSockStates[i]; j < min(state->inSockStates[i] + amountRead, STATE_URL) {
                    if (*(dtaP++) != headerMatch[j]) {
                        
            }
            if (state->inSockStates[i] == STATE_URL) {
                /* TODO */
            }
            if ((state->inSockStates[i] > STATE_URL) && (state->inSockStates[i] < STATE_HEADERS)) {
                /* TODO */
            }
            if ((state->inSockStates[i] == STATE_HEADERS) || (state->inSockStates[i] == STATE_HEADERS_FREE)) {
                while ((amountRead > 0) && (state->inSockStates[i] <= STATE_HEADERS_FREE)) {
                    if (*(dtaP++) == '\n') {
                        state->inSockStates[i]++;
                    } else {
                        if (state->inSockStates[i] == STATE_HEADERS_FREE) {
                            state->inSockStates[i]--;
                        }
                    }
                    amountRead--;
                }
            }
            if (state->inSockStates[i] == STATE_PROXY) {
                while (amountRead != 0) {
                    int h = send(state->outSocks[i], dtaP, amountRead);
                    amountRead -= h;
                    dtaP += h;
                }
            }
