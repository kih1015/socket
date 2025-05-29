#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>

#define PORTNUM 9000
#define ROOM0   0
#define MAX_CHATROOMS  20  // 총 채팅방 최대
#define MAX_USERS      10

typedef struct {
    int sock;
    char name[64];
    pthread_t thread;
    int is_conn;
    int room_no;    // 현재 참여중인 방 번호
} userinfo_t;

typedef struct {
    int   no;              
    char  name[64];        
    int   is_private;      
    int   owner_sock;      // 방장 소켓
} chatroom_t;

static userinfo_t *users[MAX_USERS];
static chatroom_t  chatrooms[MAX_CHATROOMS];
static int client_count   = 0;
static int chatroom_count = 0;

// /users 명령 응답
void resp_users(char *buf)
{
    buf[0] = '\0';
    for (int i = 0; i < client_count; i++) {
        if (users[i]->is_conn) {
            strcat(buf, users[i]->name);
            strcat(buf, ", ");
        }
    }
}

// 서버 콘솔에 유저정보 출력
void show_userinfo(void)
{
    printf("%2s\t%16s\t%12s\t%s\n", "SD", "NAME", "STATUS", "ROOM");
    printf("========================================================\n");
    for (int i = 0; i < client_count; i++) {
        printf("%02d\t%16s\t%12s\t%02d\n",
            users[i]->sock,
            users[i]->name,
            users[i]->is_conn ? "connected" : "disconnected",
            users[i]->room_no);
    }
}

// /chats 명령 응답
void resp_chatrooms(char *buf)
{
    buf[0] = '\0';
    if (chatroom_count == 0) {
        strcpy(buf, "No chatrooms.\n");
        return;
    }
    for (int i = 0; i < chatroom_count; i++) {
        snprintf(buf + strlen(buf), 128, "%d - %s %s\n",
            chatrooms[i].no,
            chatrooms[i].name,
            chatrooms[i].is_private ? "(private)" : "(public)");
    }
}

// 방 생성 (초기 멤버는 user.room_no 로 관리)
int create_chatroom(const char *name, int is_private, int owner_sock)
{
    if (chatroom_count >= MAX_CHATROOMS) return -1;
    chatroom_t *r = &chatrooms[chatroom_count];
    r->no           = chatroom_count + 1;
    strncpy(r->name, name, 63);  r->name[63] = '\0';
    r->is_private   = is_private;
    r->owner_sock   = owner_sock;    // 방장 등록
    chatroom_count++;
    return r->no;
}

// 유저가 방에 들어갈 때
void join_chatroom(userinfo_t *user, int room_no)
{
    user->room_no = room_no;
}

// 유저가 방에서 나갈 때
void leave_chatroom(userinfo_t *user)
{
    user->room_no = ROOM0;
}

// 메시지 브로드캐스트: 각 user.room_no에 맞춰 전송
void broadcast_message(int room_no, const char *msg)
{
    for (int i = 0; i < client_count; i++) {
        if (users[i]->is_conn && users[i]->room_no == room_no) {
            send(users[i]->sock, msg, strlen(msg), 0);
        }
    }
}

// 클라이언트 처리 스레드
void *client_process(void *arg)
{
    userinfo_t *user = (userinfo_t*)arg;
    char buf[512], out[512];
    int rb;

    send(user->sock, "Welcome! Input user name: ", 28, 0);
    rb = recv(user->sock, buf, sizeof(buf)-1, 0);
    buf[rb] = '\0'; strtok(buf, "\n");
    strncpy(user->name, buf, 63);
    user->is_conn = 1;
    user->room_no = ROOM0;

    while ((rb = recv(user->sock, buf, sizeof(buf)-1, 0)) > 0) {
        buf[rb] = '\0'; strtok(buf, "\n");
        char cmd_buf[512];
        strcpy(cmd_buf, buf);
        char *cmd = strtok(cmd_buf, " ");

        if (cmd && cmd[0] == '/') {
            if (!strcmp(cmd, "/users")) {
                char list[256]; resp_users(list);
                send(user->sock, list, strlen(list), 0);
            }
            else if (!strcmp(cmd, "/new")) {
                char *rname = strtok(NULL, "");
                if (!rname) {
                    send(user->sock, "Usage: /new <name>\n", 21, 0);
                } else {
                    // user->sock을 owner_sock으로 넘김
                    int no = create_chatroom(rname, 0, user->sock);
                    snprintf(out, sizeof(out),
                            "Chatroom '%s' created (#%d)\n", rname, no);
                    send(user->sock, out, strlen(out), 0);
                }
            }
            else if (!strcmp(cmd, "/start")) {
                char *id = strtok(NULL, " ");
                if (!id) continue;
                userinfo_t *peer = NULL;
                for (int i = 0; i < client_count; i++) {
                    if (!strcmp(users[i]->name, id)) { peer = users[i]; break; }
                }
                if (peer) {
                    char pname[128];
                    snprintf(pname, sizeof(pname), "%s-%s",
                            user->name, peer->name);
                    int no = create_chatroom(pname, 1, user->sock);
                    user->room_no = peer->room_no = no;
                    snprintf(out, sizeof(out),
                            "Private chat created (#%d)\n", no);
                    send(user->sock, out, strlen(out), 0);
                    send(peer->sock, out, strlen(out), 0);
                }
            }
            else if (!strcmp(cmd, "/chats")) {
                char list[512]; resp_chatrooms(list);
                send(user->sock, list, strlen(list), 0);
            }
            else if (!strcmp(cmd, "/enter")) {
                char *num_s = strtok(NULL, " ");
                int no = num_s ? atoi(num_s) : 0;
                if (no <= 0 || no > chatroom_count) send(user->sock, "Invalid room#\n", 14, 0);
                else {
                    join_chatroom(user, no);
                    snprintf(out, sizeof(out), "Entered room #%d\n", no);
                    send(user->sock, out, strlen(out), 0);
                }
            }
            else if (!strcmp(cmd, "/exit")) {
                leave_chatroom(user);
                send(user->sock, "Exited room\n", 11, 0);
            }
            else if (!strcmp(cmd, "/dm")) {
                char *id = strtok(NULL, " ");
                char *msg = strtok(NULL, "");
                if (!id || !msg) { send(user->sock, "Usage: /dm <user> <msg>\n", 25, 0); continue; }
                snprintf(out, sizeof(out), "[DM %s->%s] %s\n", user->name, id, msg);
                int found = 0;
                for (int i = 0; i < client_count; i++) {
                    if (users[i]->is_conn && !strcmp(users[i]->name, id)) {
                        send(users[i]->sock, out, strlen(out), 0);
                        found = 1;
                        break;
                    }
                }
                if (!found) send(user->sock, "User not found\n", 15, 0);
            } 
            else if (!strcmp(cmd, "/close")) {
                int rn = user->room_no;
                if (rn == ROOM0) {
                    send(user->sock, "Not in a room\n", 14, 0);
                } else {
                    chatroom_t *room = &chatrooms[rn - 1];
                    if (room->owner_sock != user->sock) {
                        send(user->sock,
                            "Only the room owner can close this room\n",
                            40, 0);
                    } else {
                        // 참여자 모두 방 나가기 처리
                        for (int i = 0; i < client_count; i++) {
                            if (users[i]->room_no == rn) {
                                leave_chatroom(users[i]);
                                send(users[i]->sock,
                                    "Room has been closed by the owner\n",
                                    34, 0);
                            }
                        }
                    }
                }
            }
        } else {
            // 일반 메시지 브로드캐스트
            snprintf(out, sizeof(out), "[%s] %s\n", user->name, buf);
            broadcast_message(user->room_no, out);
        }
    }

    user->is_conn = 0;
    leave_chatroom(user);
    close(user->sock);
    return NULL;
}

int main() {
    int sd, ns;
    struct sockaddr_in sin, cli;
    socklen_t clilen = sizeof(cli);

    sd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = INADDR_ANY;
    bind(sd, (struct sockaddr*)&sin, sizeof(sin));
    listen(sd, 5);

    int epfd = epoll_create1(0);
    struct epoll_event ev, events[10];
    ev.events = EPOLLIN; ev.data.fd = sd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);
    ev.events = EPOLLIN; ev.data.fd = 0;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);

    printf("> "); fflush(stdout);

    while (1) {
        int n = epoll_wait(epfd, events, 10, -1);
        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == sd) {
                ns = accept(sd, (struct sockaddr*)&cli, &clilen);
                userinfo_t *u = calloc(1, sizeof(*u));
                u->sock = ns;
                users[client_count++] = u;
                pthread_create(&u->thread, NULL, client_process, u);
            } else if (events[i].data.fd == 0) {
                char cmd[64];
                fgets(cmd, sizeof(cmd), stdin);
                strtok(cmd, " \n");
                if (!strcmp(cmd, "users")) show_userinfo();
                else if (!strcmp(cmd, "chats")) {
                    char list[512]; resp_chatrooms(list);
                    printf("%s", list);
                }
                printf("> "); fflush(stdout);
            }
        }
    }

    close(sd);
    return 0;
}
