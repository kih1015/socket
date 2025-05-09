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
#define MAX_USERS 10

// 클라이언트 정보 구조체
typedef struct {
    int sock;
    char name[NAME_LEN];
    pthread_t thread;
    int is_conn;
} userinfo_t;

// 최대 10명까지 관리
typedef struct {
    userinfo_t *list[MAX_USERS];
    int count;
} user_manager_t;

static user_manager_t manager = {.count = 0};

// 사용자 목록 출력
void show_userinfo() {
    printf("=== 사용자 목록 ===\n");
    for (int i = 0; i < manager.count; ++i) {
        userinfo_t *u = manager.list[i];
        if (u) {
            printf("[%d] %16s\tfd=%d\t%s\n",
                   i,
                   u->name,
                   u->sock,
                   u->is_conn ? "connected" : "disconnected");
        }
    }
}

// 클라이언트 처리 스레드
void *client_process(void *arg) {
    userinfo_t *user = (userinfo_t *)arg;
    char buf[256];
    char buf2[256];
    int len;

    // 1) 환영 메시지 및 이름 요청
    snprintf(buf, sizeof(buf), "Welcome!\nInput user name: ");
    send(user->sock, buf, strlen(buf), 0);

    // 2) 이름 수신
    len = recv(user->sock, buf, NAME_LEN - 1, 0);
    if (len <= 0) {
        user->is_conn = 0;
        close(user->sock);
        free(user);
        return NULL;
    }
    buf[len] = '\0';
    strtok(buf, "\r\n");
    strncpy(user->name, buf, NAME_LEN - 1);
    user->name[NAME_LEN - 1] = '\0';
    printf("user name: %s\n", user->name);

    // 3) 에코 및 브로드캐스트
    while (1) {
        int rb = recv(user->sock, buf, sizeof(buf) - 1, 0);
        if (rb <= 0) {
            user->is_conn = 0;
            printf("user %s disconnected\n", user->name);
            break;
        }
        buf[rb] = '\0';
        // 모든 연결된 클라이언트에 전송
        for (int i = 0; i < manager.count; ++i) {
            userinfo_t *u = manager.list[i];
            if (u && u->is_conn) {
                snprintf(buf2, sizeof(buf2), "[%s] %s\n", user->name, buf);
                send(u->sock, buf2, strlen(buf2), 0);
            }
        }
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
    struct epoll_event ev, events[MAX_USERS];

    // 1) 서버 소켓 생성
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2) 바인딩 및 리슨
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(PORTNUM);
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(sd, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // 3) epoll 생성 및 서버 FD 등록
    int epfd = epoll_create1(0);
    ev.events = EPOLLIN;
    ev.data.fd = sd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);

    // 4) stdin FD 등록
    ev.events = EPOLLIN;
    ev.data.fd = STDIN_FILENO;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    printf("포트 %d에서 채팅 서버 시작\n", PORTNUM);

    // 5) 이벤트 루프
    while (1) {
        int nfd = epoll_wait(epfd, events, MAX_USERS, -1);
        if (nfd <= 0) continue;
        for (int i = 0; i < nfd; ++i) {
            int fd = events[i].data.fd;
            if (fd == sd) {
                // 새 클라이언트 연결 수락
                ns = accept(sd, (struct sockaddr *)&cli, &clientlen);
                if (ns == -1) continue;
                // 사용자 구조체 할당 및 초기화
                userinfo_t *user = malloc(sizeof(userinfo_t));
                user->sock = ns;
                user->is_conn = 1;
                memset(user->name, 0, NAME_LEN);
                // 스레드 실행
                pthread_create(&user->thread, NULL, client_process, user);
                // 매니저에 등록
                if (manager.count < MAX_USERS) {
                    manager.list[manager.count++] = user;
                } else {
                    close(ns);
                    free(user);
                }

            } else if (fd == STDIN_FILENO) {
                // stdin 명령 처리
                char command[64];
                if (!fgets(command, sizeof(command), stdin)) continue;
                strtok(command, "\r\n");
                if (strcmp(command, "users") == 0) {
                    show_userinfo();
                }
                printf("> "); fflush(stdout);

            }
        }
    }

    close(sd);
    return 0;
}
