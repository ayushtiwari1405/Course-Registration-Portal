#include "academia.h"

// Send message to socket.
void sender(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

// Receive data into buffer. Handle empty input/connection close.
void receiver(int sock, char *buf, size_t buflen) {
    memset(buf, 0, buflen);
    int n = recv(sock, buf, buflen - 1, 0);
    if (n <= 0) {
        buf[0] = '\0';
        return;
    }
    buf[n] = '\0';
    buf[strcspn(buf, "\r\n")] = 0;
    if (strlen(buf) == 0) {
        strncpy(buf, "INVALID_INPUT", buflen - 1);
    }
}