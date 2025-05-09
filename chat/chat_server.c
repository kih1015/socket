/*
 * 간단한 C 채팅 서버 (epoll + pthread 기반)
 * 요구사항:
 * - 여러 클라이언트 동시 접속 지원
 * - 접속 시 사용자 이름 입력 요청
 * - 에코 및 브로드캐스트
 * - 명령어 "users"로 접속 사용자 목록 출력
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#define PORTNUM 9000
#define NAME_LEN 64

// 클라이언트 정보 구조체
typedef struct {
    int sock;
    char name[NAME_LEN];
    pthread_t thread;
} userinfo_t;

// 최대 10명까지 관리 (clients 이벤트 인덱스 기반)
static userinfo_t *users[10] = {0};

// 사용자 목록 출력
void show_userinfo() {
    printf("=== 사용자 목록 ===\n");
    for (int i = 0; i < 10; ++i) {
        if (users[i]) {
            printf("[%d] %s (fd=%d)\n", i, users[i]->name, users[i]->sock);
        }
    }
}

// 클라이언트 처리 스레드
void *client_process(void *arg) {
    userinfo_t *user = (userinfo_t *)arg;
    char buf[256];
    int len;

    // 1) 환영 및 이름 요청
    snprintf(buf, sizeof(buf), "Welcome!\nInput user name: ");
    send(user->sock, buf, strlen(buf), 0);

    // 2) 이름 수신
    len = recv(user->sock, buf, NAME_LEN - 1, 0);
    if (len <= 0) {
        close(user->sock);
        free(user);
        return NULL;
    }
    buf[len] = '\0';
    strtok(buf, "\r\n");
    strncpy(user->name, buf, NAME_LEN - 1);
    printf("user name: %s\n", user->name);

    // 3) 에코 루프
    while (1) {
        int rb = recv(user->sock, buf, sizeof(buf) - 1, 0);
        if (rb <= 0) break;
        buf[rb] = '\0';
        send(user->sock, buf, strlen(buf), 0);
    }

    // 4) 연결 종료
    close(user->sock);
    free(user);
    return NULL;
}

int main() {
    int sd, ns;
    struct sockaddr_in sin, cli;
    socklen_t clientlen = sizeof(cli);
    struct epoll_event ev, events[10];

    // 1) 서버 소켓 생성
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2) 바인딩과 리슨
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(PORTNUM);
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(sd, 5) == -1) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    // 3) epoll 생성 및 서버 FD 등록
    int epfd = epoll_create1(0);
    ev.events = EPOLLIN;
    ev.data.fd = sd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);

    // 4) stdin FD 등록
    ev.events = EPOLLIN;
    ev.data.fd = 0;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);

    printf("포트 %d에서 채팅 서버 시작\n", PORTNUM);

    // 5) 이벤트 루프
    while (1) {
        int nfd = epoll_wait(epfd, events, 10, -1);
        if (nfd <= 0) continue;
        for (int i = 0; i < nfd; ++i) {
            if (events[i].data.fd == sd) {
                // 새 클라이언트 연결 수락
                ns = accept(sd, (struct sockaddr *)&cli, &clientlen);
                if (ns == -1) continue;
                // 사용자 구조체 할당
                userinfo_t *user = malloc(sizeof(userinfo_t));
                user->sock = ns;
                memset(user->name, 0, NAME_LEN);
                // 스레드 실행
                pthread_create(&user->thread, NULL, client_process, user);
                users[i] = user;

            } else if (events[i].data.fd == 0) {
                // stdin 명령 처리
                char command[64];
                if (fgets(command, sizeof(command), stdin) == NULL) continue;
                strtok(command, "\n");
                if (strcmp(command, "users") == 0) {
                    show_userinfo();
                }
                printf("> "); fflush(stdout);

            } else {
                // 기존 클라이언트 에코 처리
                int fd = events[i].data.fd;
                char buf[256];
                int rb = recv(fd, buf, sizeof(buf) - 1, 0);
                if (rb > 0) {
                    buf[rb] = '\0';
                    send(fd, buf, strlen(buf), 0);
                }
            }
        }
    }

    close(sd);
    return 0;
}
