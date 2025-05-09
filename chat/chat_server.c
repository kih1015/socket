/*
 * 간단한 C 채팅 서버 (epoll + pthread 기반)
 * 요구사항:
 * - 여러 클라이언트 동시 접속 지원
 * - 접속 시 사용자 이름 입력 요청
 * - 메시지를 에코 및 브로드캐스트
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
#define MAX_EVENTS 10
#define NAME_LEN 64

// 클라이언트 정보 구조체
typedef struct {
    int sock;
    char name[NAME_LEN];
    pthread_t thread;
} userinfo_t;

// 최대 10명까지 관리
static userinfo_t *users[MAX_EVENTS];

// 클라이언트 처리 함수
void *client_process(void *arg) {
    userinfo_t *user = (userinfo_t *)arg;
    char buf[256];
    int len;

    // 이름 요청
    snprintf(buf, 255, "Welcome!\nInput user name: ");
    send(user->sock, buf, strlen(buf), 0);

    // 이름 수신
    len = recv(user->sock, buf, NAME_LEN - 1, 0);
    if (len <= 0) {
        close(user->sock);
        free(user);
        return NULL;
    }
    buf[len] = '\0';
    printf("user name: %s\n", buf);
    strncpy(user->name, buf, NAME_LEN - 1);

    // 에코 루프
    while (1) {
        int rb = recv(user->sock, buf, sizeof(buf) - 1, 0);
        if (rb <= 0) break;
        buf[rb] = '\0';
        send(user->sock, buf, strlen(buf), 0);
    }

    close(user->sock);
    free(user);
    return NULL;
}

int main() {
    int sd, ns, epfd, nfds;
    struct sockaddr_in sin, cli;
    socklen_t clientlen = sizeof(cli);
    struct epoll_event ev, events[MAX_EVENTS];

    // 소켓 생성
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 주소 재사용
    int opt = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 바인딩
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
        perror("bind");
        exit(1);
    }

    // 리스닝
    if (listen(sd, 5) == -1) {
        perror("listen");
        exit(1);
    }

    // epoll 생성 및 등록
    epfd = epoll_create1(0);
    ev.events = EPOLLIN;
    ev.data.fd = sd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);

    printf("포트 %d에서 채팅 서버 시작\n", PORTNUM);

    while (1) {
        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == sd) {
                // 새 연결
                ns = accept(sd, (struct sockaddr *)&cli, &clientlen);
                if (ns == -1) continue;

                // 클라이언트 구조체 할당 및 초기화
                userinfo_t *user = malloc(sizeof(userinfo_t));
                user->sock = ns;
                memset(user->name, 0, NAME_LEN);

                // 스레드 생성
                pthread_create(&user->thread, NULL, client_process, user);
                // 관리 배열에 추가
                users[i] = user;

            } else {
                // 클라이언트 데이터 처리 (간단 에코)
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
