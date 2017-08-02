#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>

#define MAX_CONS 8
#define DEFAULT_URL_STORE 64
#define MAX_URL_STORE 2048
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

char headerMatch[] = "CONNECT __HTTP/1.1\r";
#define STATE_URL_SETUP 8
#define STATE_URL 9
#define STATE_HEADERS 19
#define STATE_HEADERS_FREE 20
#define STATE_PROXY_INIT 21
#define STATE_PROXY 22

char nopeNopeNope[] = "HTTP/1.1 400 Bad Request\rServer: tcproxy\rContent-Length: 0\r\r"

char ftpString[] = "ftp://"
#define FTP_LEN 6

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

void _cleanup(struct tcproxy *state, int sock, unsigned char bringDown) {
    if (state->inSockStates[sock] == STATE_PROXY) {
        close(state->outSocks[sock]);
    } else if (state->inSockStates[sock] >= STATE_URL) {
#ifdef ZERO_SECURE
        zero(state->urls[sock].mem, state->urls[sock].size);
#endif
        free(state->urls[sock].mem);
    }
    close(state->inSocks[sock]);
    if (bringDown) {
        memmove(state->inSocks + sock, state->inSocks + sock + 1, 2 * (state->cons - sock) - 2);
        memmove(state->outSocks + sock, state->outSocks + sock + 1, 2 * (state->cons - sock) - 2);
        memmove(state->inSockStates + sock, state->inSockStates + sock + 1, state->cons - sock - 1);
        memmove(state->urls + sock, state->urls + sock + 1, (state->cons - sock - 1) * sizeof(struct flex_mem));
        state->cons--;
        sock = state->cons - 1;
    }
#ifdef ZERO_SECURE
    state->inSocks[sock] = 0;
    state->outSocks[sock] = 0;
    state->inSockStates[sock] = 0;
    zero(state->urls + sock, sizeof(struct flex_mem));
#endif
}

#define cleanup(x, y) _cleanup(x, y, 1)

int getURLSock(struct flex_mem *url) {
    

int sendMessage(int sock, char *buff, unsigned int size) {
    int r;
    while (size > 0) {
        r = send(sock, buff, size);
        if (r > 0) {
            buff += r;
            size += r;
        }
        if ((r == -1) && (errno != EINTR)) {
            return errno;
        }
    }
}

void _cleanupWithSend(struct tcproxy *state, int sock, unsigned char bringDown) {
    sendMessage(sock, nopeNopeNope, strlen(nopeNopeNope));
    _cleanup(state, sock, bringDown);
}

#define cleanupWithSend(x, y) _cleanupWithSend(x, y, 1)

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
            cleanup(state, i);
            i--;
            continue;
        } else {
            dtaP = buff;
            /* Before url */
            while ((state->inSockStates[i] < STATE_URL_SETUP) && (amountRead > 0)) {
                if (*(dtaP++) != headerMatch[state->inSockStates[i]]) {
                    cleanupWithSend(state, i);
                    i--;
                    continue;
                }
                amountRead--;
                state->inSockStates[i]++;
            }
            if (state->inSockStates[i] == STATE_URL_SETUP) {
                state->urls[i].mem = malloc(DEFAULT_URL_STORE);
                state->urls[i].size = DEFAULT_URL_STORE;
                state->urls[i].pos = 0;
                state->inSockStates[i]++;
            }
            while ((state->inSockStates[i] == STATE_URL) && (amountRead > 0)) {
                char in = *(dtaP++);
                if (in == ' ') {
                    state->inSockStates++;
                } else {
                    if (state->urls[i].size = state->urls[i].size) {
                        unsigned int ne = state->urls[i].size * 2;
                        if (ne > MAX_URL_STORE) {
                            cleanupWithSend(state, i);
                            i--;
                            continue;
                        } else {
                            state->urls[i].size = ne;
                        }
                        state->urls[i].mem = realloc(state->urls[i].mem, state->urls[i].size);
                    }
                    state->urls[i].mem[state->urls[i].pos++] = in;
                }
            }
            if (state->inSockStates[i] > STATE_URL) {
                while ((state->inSockStates[i] < STATE_HEADERS) && (amountRead > 0)) {
                    if (*(dtaP++) != headerMatch[state->inSockStates[i]]) {
                        cleanupWithSend(state, i);
                        i--;
                        continue;
                    }
                    amountRead++;
                    state->inSockStates[i]++;
                }
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
            if (state->inSockStates[i] == STATE_PROXY_INIT) {
                if (memcmp(
            }
            if (state->inSockStates[i] == STATE_PROXY) {
                if (sendMessage(state->outSocks[i], dtaP, amountRead) < 0) {
                    cleanupWithSend(state, state->inSocks[i]);
                    i--;
                    continue;
                }
            }
        }
    }
}
