#include "../tools.h"

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in rcvServAddr;
    unsigned short rcvServPort;
    char *servIP;
    int bytesRcvd;
    char buffer[BUFF_LEN];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];
    rcvServPort = atoi(argv[2]);

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        exit_error("socket() failed");
    }

    memset(&rcvServAddr, 0, sizeof(rcvServAddr));
    rcvServAddr.sin_family = AF_INET;
    rcvServAddr.sin_addr.s_addr = inet_addr(servIP);
    rcvServAddr.sin_port = htons(rcvServPort);

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        exit_error("socket() failed\n");
    }

    if (connect(sock, (struct sockaddr *) &rcvServAddr, sizeof(rcvServAddr)) < 0) {
        exit_error("connect() failed");
    }

    if ((bytesRcvd = recv(sock, buffer, sizeof(buffer) - 1, 0)) < 0) {
        exit_error("recv() failed");
    }

    for (; bytesRcvd > 0;) {
        buffer[bytesRcvd] = '\0';
        printf("%s\n", buffer);
        usleep(100 * 1000);
        if ((bytesRcvd = recv(sock, buffer, sizeof(buffer) - 1, 0)) < 0) {
            exit_error("recv() failed");
        }
    }

    close(sock);
    exit(0);
}