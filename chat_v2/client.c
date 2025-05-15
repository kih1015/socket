#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 9000
#define BUF_SIZE    512

static int sockfd;

// 서버로부터 메시지를 수신해 출력하는 스레드
void *recv_handler(void *arg) {
    char buf[BUF_SIZE];
    int n;
    while ((n = recv(sockfd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
        fflush(stdout);
    }
    // 서버 연결 종료 시
    printf("서버와의 연결이 끊어졌습니다.\n");
    exit(0);
}

int main() {
    struct sockaddr_in serv_addr;
    pthread_t recv_thread;
    char send_buf[BUF_SIZE];

    // 소켓 생성
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 서버 주소 설정
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // 서버에 연결
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    // 수신 스레드 생성
    if (pthread_create(&recv_thread, NULL, recv_handler, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // 사용자 입력을 서버로 전송
    while (fgets(send_buf, sizeof(send_buf), stdin) != NULL) {
        size_t len = strlen(send_buf);
        if (len > 0 && send_buf[len - 1] == '\n') {
            send_buf[len - 1] = '\0';
        }
        if (send(sockfd, send_buf, strlen(send_buf), 0) < 0) {
            perror("send");
            break;
        }
    }

    // 입력 종료 시 종료 처리
    close(sockfd);
    return 0;
}
