#include "../tools.h"

int sock;

void handle_exit() {
    close(sock);
    exit(0);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_exit);
    struct sockaddr_in rcvServAddr;
    unsigned short rcvServPort;
    char *servIP;
    char rcvBuff[1];
    int bytesRcvd, totalBytesRcvd;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];
    rcvServPort = atoi(argv[2]);

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        exit_error("socket() failed");
    }

    memset(&rcvServAddr, 0, sizeof(rcvServAddr));
    rcvServAddr.sin_family = AF_INET;
    rcvServAddr.sin_addr.s_addr = inet_addr(servIP);
    rcvServAddr.sin_port = htons(rcvServPort);

    char c = BEAR;

    while (1) {
        usleep(10 * 1000);
        if (sendto(sock, &c, 1, 0, (struct sockaddr *) &rcvServAddr, sizeof(rcvServAddr)) != 1) {
            exit_error("sendto() sent a different number of bytes than expected");
        }

        totalBytesRcvd = 0;
        while (totalBytesRcvd < 1) {
            if ((bytesRcvd = recvfrom(sock, rcvBuff, 1, 0, NULL, NULL)) <= 0) {
                exit_error("recvfrom() failed or connection closed prematurely");
            }
            totalBytesRcvd += bytesRcvd;
            if (rcvBuff[0] == EMPTY) {
                continue;
            } else if (rcvBuff[0] == STUNG) {
                printf("Stung\n");
                int sleep_time = rand() % 1500 + 500;
                usleep(sleep_time * 1000);
            } else if (rcvBuff[0] == STOLE) {
                printf("Stole\n");
            } else {
                printf("Uncorrect value\n");
            }
        }
    }
}
