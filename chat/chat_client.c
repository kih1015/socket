/*
 * Simple Chat Client in C
 * Usage: chat_client <server_ip> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUF_SIZE   1024
#define NAME_LEN   32

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char send_buf[BUF_SIZE];
    char recv_buf[BUF_SIZE];
    fd_set master_set, read_fds;
    int fdmax;
    int nbytes;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 소켓 생성
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // 서버 연결
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // fd_set 초기화
    FD_ZERO(&master_set);
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &master_set);
    FD_SET(sockfd, &master_set);
    fdmax = sockfd;

    // 서버로부터 닉네임 요청 메시지 수신
    if ((nbytes = recv(sockfd, recv_buf, BUF_SIZE-1, 0)) <= 0) {
        perror("recv");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    recv_buf[nbytes] = '\0';
    printf("%s", recv_buf);

    // 닉네임 입력 및 전송
    if (fgets(send_buf, NAME_LEN, stdin) == NULL) {
        fprintf(stderr, "No nickname entered.\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    send_buf[strcspn(send_buf, "\r\n")] = '\0';
    send(sockfd, send_buf, strlen(send_buf), 0);

    // 메인 루프: stdin과 서버 연결 양쪽 감시
    while (1) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }

        // stdin 입력 처리
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            if (fgets(send_buf, BUF_SIZE, stdin) == NULL) break;
            send_buf[strcspn(send_buf, "\r\n")] = '\0';
            if (strcmp(send_buf, "/quit") == 0) {
                send(sockfd, send_buf, strlen(send_buf), 0);
                break;
            }
            send(sockfd, send_buf, strlen(send_buf), 0);
        }

        // 서버 메시지 처리
        if (FD_ISSET(sockfd, &read_fds)) {
            if ((nbytes = recv(sockfd, recv_buf, BUF_SIZE-1, 0)) <= 0) {
                printf("서버 연결이 종료되었습니다.\n");
                break;
            }
            recv_buf[nbytes] = '\0';
            printf("%s", recv_buf);
        }
    }

    close(sockfd);
    return 0;
}
