#include "../tools.h"

void handle_exit() {
    exit(0);
}

void process_bee(int sock, int n) {
    int totalBytesRcvd, bytesRcvd, sleep_time;
    char c;
    char rcvBuff[1];
    while (1) {
        c = BEE_OUT;
        if (send(sock, &c, 1, 0) != 1) {
            exit_error("send() sent a different number of bytes than expected\n");
        }

        totalBytesRcvd = 0;
        while (totalBytesRcvd < 1) {
            if ((bytesRcvd = recv(sock, rcvBuff, 1, 0)) < 0) {
                exit_error("recv1() failed or connection closed prematurely\n");
            }
            totalBytesRcvd += bytesRcvd;
            if (rcvBuff[0] != BEE_OUT) {
                exit_error("Receive uncorrect status\n");
            } else {
                printf("%d bee flew away\n", n + 1);
            }
        }
        sleep_time = rand() % 1500 + 500;
        usleep(sleep_time * 1000);

        c = BEE_IN;
        if (send(sock, &c, 1, 0) != 1) {
            exit_error("send() sent a different number of bytes than expected");
        }

        totalBytesRcvd = 0;
        while (totalBytesRcvd < 1) {
            if ((bytesRcvd = recv(sock, rcvBuff, 1, 0)) < 0) {
                exit_error("recv2() failed or connection closed prematurely\n");
            }
            totalBytesRcvd += bytesRcvd;
            if (rcvBuff[0] != BEE_IN) {
                exit_error("Receive uncorrect status\n");
            } else {
                printf("%d bee arrivied\n", n + 1);
            }
        }
        sleep_time = rand() % 100 + 100;
        usleep(sleep_time * 1000);
    }
}

int main(int argc, char *argv[]) {
    prev_bee = signal(SIGINT, handle_exit);
    struct sockaddr_in echoServAddr;
    unsigned short echoServPort;
    char *servIP;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n", argv[0]);
        exit(1);
    }

    servIP = argv[1];
    echoServPort = atoi(argv[2]);

    pid_t processID;

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);
    echoServAddr.sin_port = htons(echoServPort);

    for (int i = 0; i < N_BEES; ++i) {
        if ((processID = fork()) < 0) {
            exit_error("fork() failed\n");
        } else if (processID == 0) {
            int sock;
            if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
                exit_error("socket() failed\n");
            }
            if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0) {
                exit_error("connect() failed\n");
            }
            signal(SIGINT, prev_bee);
            process_bee(sock, i);
        }
    }
    while (1) {};
}
