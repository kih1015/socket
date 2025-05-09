/*
 * 간단한 C 채팅 서버 (epoll + pthread 기반)
 * 요구사항:
 * - 여러 클라이언트 동시 접속 지원
 * - 접속 시 사용자 이름 입력 요청
 * - 에코 및 브로드캐스트
 * - 클라이언트에서 "/users" 입력 시 목록 전송
 * - 클라이언트에서 "/dm <id> <메시지>" 입력 시 해당 사용자에게 DM 전송
 * - 클라이언트에서 "/start <id>" 입력 시 1:1 대화방 생성
 * - 서버 콘솔에서 "users" 입력 시 목록 출력
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

#define PORTNUM    9000
#define NAME_LEN   64
#define MAX_USERS  10

// 클라이언트 정보 구조체
typedef struct {
    int    sock;
    char   name[NAME_LEN];
    pthread_t thread;
    int    is_conn;
    int    chat_room;   // 0: 전체 채팅, >0: 1:1 방 번호
} userinfo_t;

// 1:1 방 정보 구조체
typedef struct {
    int          no;           // 방 번호
    userinfo_t * member[2];    // 두 명
} roominfo_t;

// 전역 사용자·방 배열 및 개수
static userinfo_t * users[MAX_USERS]   = {0};
static int         client_num         = 0;
static roominfo_t  rooms[MAX_USERS];  // 0번 방은 쓰지 않음
static int         room_count         = 0;

// 서버 콘솔: 사용자 목록 출력
void show_userinfo() {
    printf("=== 사용자 목록 ===\n");
    printf(" idx |      NAME       | fd  |   상태   | 방번호\n");
    printf("-----+-----------------+-----+----------+-------\n");
    for (int i = 0; i < client_num; ++i) {
        userinfo_t *u = users[i];
        if (u) {
            printf("%4d | %15s | %3d | %s | %3d\n",
                   i,
                   u->name[0] ? u->name : "(no name)",
                   u->sock,
                   u->is_conn ? "connected" : "disc.",
                   u->chat_room);
        }
    }
    printf("\n");
}

// 클라이언트 전용: 사용자 목록 전송
void send_user_list(int dest_sock) {
    char buf[256];
    int  len;
    len = snprintf(buf, sizeof(buf), "=== 현재 접속자 목록 ===\n");
    send(dest_sock, buf, len, 0);
    for (int i = 0; i < client_num; ++i) {
        userinfo_t *u = users[i];
        if (u) {
            len = snprintf(buf, sizeof(buf), "[%d] %s\n", i,
                           u->name[0] ? u->name : "(no name)");
            send(dest_sock, buf, len, 0);
        }
    }
}

// 클라이언트 처리 스레드
void *client_process(void *arg) {
    userinfo_t *user = (userinfo_t *)arg;
    char        buf[512], buf2[512], msg[640];
    int         rb, len;

    // 1) 환영 메시지 및 닉네임 요청
    send(user->sock, "Welcome!\nInput user name: ", 25, 0);
    rb = recv(user->sock, buf, NAME_LEN - 1, 0);
    if (rb <= 0) {
        user->is_conn = 0;
        close(user->sock);
        return NULL;
    }
    buf[rb] = '\0';
    strtok(buf, "\r\n");
    strncpy(user->name, buf, NAME_LEN - 1);
    user->name[NAME_LEN - 1] = '\0';
    user->chat_room = 0;  // 기본 방은 0(전체)
    printf("user name: %s\n", user->name);

    // 2) 메시지 루프
    while (1) {
        rb = recv(user->sock, buf, sizeof(buf) - 1, 0);
        if (rb <= 0) {
            user->is_conn = 0;
            close(user->sock);
            printf("user %s disconnected\n", user->name);
            return NULL;
        }
        buf[rb] = '\0';
        strtok(buf, "\r\n");   // 개행 제거

        // 명령어 체크: starts with '/'
        if (buf[0] == '/') {
            // tokenize 기준 문자열 복사
            strcpy(buf2, buf);
            strtok(buf2, " ");  // buf2에 첫 토큰

            // 1) /users
            if (strcmp(buf2, "/users") == 0) {
                send_user_list(user->sock);
            }
            // 2) /dm <id> <message>
            else if (strcmp(buf2, "/dm") == 0) {
                char *id_str = strtok(buf + 4, " ");
                char *dm_msg = strtok(NULL, "");
                if (id_str && dm_msg) {
                    int tid = atoi(id_str);
                    if (tid >= 0 && tid < client_num
                     && users[tid] && users[tid]->is_conn) {
                        len = snprintf(msg, sizeof(msg),
                                "[DM from %s] %s\n",
                                user->name, dm_msg);
                        send(users[tid]->sock, msg, len, 0);
                    } else {
                        send(user->sock,
                         "Invalid user id or user not connected\n",
                         40, 0);
                    }
                } else {
                    send(user->sock,
                         "Usage: /dm <id> <message>\n",
                         30, 0);
                }
            }
            // 3) /start <id> → 1:1 방 생성 및 참가
            else if (strcmp(buf2, "/start") == 0) {
                char *id_str = strtok(buf + 7, " ");
                if (id_str) {
                    int tid = atoi(id_str);
                    if (tid >= 0 && tid < client_num
                     && users[tid] && users[tid]->is_conn) {
                        if (room_count + 1 < MAX_USERS) {
                            int rno = ++room_count;
                            rooms[rno].no              = rno;
                            rooms[rno].member[0]       = user;
                            rooms[rno].member[1]       = users[tid];
                            user->chat_room            = rno;
                            users[tid]->chat_room      = rno;
                        } else {
                            send(user->sock,
                              "Cannot create more rooms\n",
                              strlen("Cannot create more rooms\n"), 0);
                        }
                    } else {
                        send(user->sock,
                          "Invalid user id or not connected\n",
                          strlen("Invalid user id or not connected\n"), 0);
                    }
                } else {
                    send(user->sock,
                      "Usage: /start <id>\n",
                      strlen("Usage: /start <id>\n"), 0);
                }
            }
            // 명령어이므로 여기서 건너뛴다
            continue;
        }

        // 일반 메시지: 방 번호에 따라 라우팅
        if (user->chat_room == 0) {
            // 0번 방(전체) 일 때 → 전체방 사용자에게만
            len = snprintf(msg, sizeof(msg),
                          "[%s] %s\n", user->name, buf);
            for (int i = 0; i < client_num; ++i) {
                if (users[i] && users[i]->is_conn
                 && users[i]->chat_room == 0) {
                    send(users[i]->sock, msg, len, 0);
                }
            }
        } else {
            // 1:1 방 → 두 멤버에게만
            roominfo_t *r = &rooms[user->chat_room];
            for (int m = 0; m < 2; ++m) {
                userinfo_t *u = r->member[m];
                if (u && u->is_conn) {
                    len = snprintf(msg, sizeof(msg),
                                  "[Room%d][%s] %s\n",
                                  r->no, user->name, buf);
                    send(u->sock, msg, len, 0);
                }
            }
        }
    }

    return NULL;
}

int main() {
    int sd, ns;
    struct sockaddr_in sin, cli;
    socklen_t clientlen = sizeof(cli);
    struct epoll_event ev, events[MAX_USERS];
    int epfd;

    // 1) 서버 소켓 생성 및 옵션
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket"); exit(EXIT_FAILURE);
    }
    int opt = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2) 바인딩/리스닝
    memset(&sin, 0, sizeof(sin));
    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port        = htons(PORTNUM);
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(sd, 5) == -1) {
        perror("listen"); exit(EXIT_FAILURE);
    }

    // 3) epoll 생성 및 서버·STDIN 등록
    epfd = epoll_create1(0);
    ev.events   = EPOLLIN;
    ev.data.fd  = sd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);
    ev.data.fd  = STDIN_FILENO;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    printf("포트 %d에서 채팅 서버 시작\n", PORTNUM);

    // 4) 이벤트 루프
    while (1) {
        int nfd = epoll_wait(epfd, events, MAX_USERS, -1);
        if (nfd <= 0) continue;
        for (int i = 0; i < nfd; ++i) {
            int fd = events[i].data.fd;
            if (fd == sd) {
                // 새 연결
                ns = accept(sd, (struct sockaddr *)&cli, &clientlen);
                if (ns < 0) continue;
                // 유저 구조체 생성
                userinfo_t *u = malloc(sizeof(*u));
                u->sock      = ns;
                u->is_conn   = 1;
                u->chat_room = 0;
                memset(u->name, 0, NAME_LEN);
                // 배열에 추가
                if (client_num < MAX_USERS) {
                    users[client_num++] = u;
                    pthread_create(&u->thread, NULL, client_process, u);
                } else {
                    close(ns);
                    free(u);
                }
            } else if (fd == STDIN_FILENO) {
                // 서버 콘솔
                char cmd[64];
                if (!fgets(cmd, sizeof(cmd), stdin)) continue;
                strtok(cmd, "\r\n");
                if (strcmp(cmd, "users") == 0) {
                    show_userinfo();
                }
                printf("> "); fflush(stdout);
            }
        }
    }

    close(sd);
    return 0;
}
