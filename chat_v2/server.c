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
#define MAX_CHATROOMS  10

typedef struct {
    int sock;
    char name[64];
    pthread_t thread;
    int is_conn;
    int chat_room;
} userinfo_t;

typedef struct {
    int no;
    userinfo_t *member[2];
} roominfo_t;

typedef struct {
    int   no;             // 채팅방 번호 (1부터 시작)
    char  name[64];       // 채팅방 이름
} chatroom_t;

static userinfo_t *users[10];
static roominfo_t rooms[10];
static int client_num = 0;
static int room_num = 0;

static chatroom_t chatrooms[MAX_CHATROOMS];
static int        chatroom_count = 0;

void show_userinfo(void)
{
    int i;

    printf("%2s\t%16s\t%12s\t%s\n", "SD", "NAME", "CONNECTION", "ROOM");
    printf("=========================================================\n");

    for (i = 0; i < client_num; i++) {
        printf("%02d\t%16s\t%12s\t%02d\n", 
            users[i]->sock, 
            users[i]->name, 
            users[i]->is_conn ? "connected":"disconnected",
            users[i]->chat_room
            );
    }
}

void show_roominfo(void)
{
    /* TODO: print rooms array */
}

void resp_users(char *buf)
{
    buf[0] = '\0';

    for (int i = 0; i < client_num; i++)
    {
        strcat(buf, users[i]->name);
        strcat(buf, ", ");
    }
}

void create_room(userinfo_t *user1, userinfo_t *user2)
{
    rooms[room_num].no = room_num + 1;
    rooms[room_num].member[0] = user1;
    rooms[room_num].member[1] = user2;

    user1->chat_room = room_num + 1;
    user2->chat_room = room_num + 1;
    room_num++;
}

// --- 채팅방 목록 문자열 생성 함수 (client_process 내 /chats용)
void resp_chatrooms(char *buf)
{
    buf[0] = '\0';
    if (chatroom_count == 0) {
        strcpy(buf, "No chatrooms.\n");
        return;
    }
    for (int i = 0; i < chatroom_count; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%d - %s\n",
                 chatrooms[i].no,
                 chatrooms[i].name);
        strcat(buf, line);
    }
}

// --- 채팅방 생성 함수 (client_process 내 /new용)
int create_chatroom(const char *name)
{
    if (chatroom_count >= MAX_CHATROOMS) return -1;
    chatrooms[chatroom_count].no = chatroom_count + 1;
    strncpy(chatrooms[chatroom_count].name, name, 63);
    chatrooms[chatroom_count].name[63] = '\0';
    chatroom_count++;
    return chatroom_count;  // 생성된 방 번호
}

// --- 서버 콘솔용 채팅방 목록 출력 (main() 내 콘솔명령용)
void show_chatrooms(void)
{
    printf("Chatrooms:\n");
    printf("================================================\n");
    if (chatroom_count == 0) {
        printf("  (none)\n");
    } else {
        for (int i = 0; i < chatroom_count; i++) {
            printf("  %2d - %s\n",
                   chatrooms[i].no,
                   chatrooms[i].name);
        }
    }
    printf("================================================\n");
}

void *client_process(void *arg)
{
    userinfo_t *user = (userinfo_t *)arg;
    char buf[256];
    char buf2[256];
    int len;

    snprintf(buf, 255, "Welcome!\n Input user name: ");
    send(user->sock, buf, strlen(buf), 0);
    len = recv(user->sock, buf, 63, 0);
    buf[len] = 0;
    printf("user name: %s\n", strtok(buf, "\n"));

    strncpy(user->name, buf, 63);

    while (1) {
        int rb = recv(user->sock, buf, sizeof(buf), 0);
        if (rb <= 0) {
            user->is_conn = 0;
            printf("user %s disconnected\n", user->name);
            pthread_exit(NULL);
        }
        buf[rb] = '\0';
        strtok(buf, "\n");
        strcpy(buf2, buf);
        strtok(buf2, " ");

        /* command */
        if (buf[0] == '/') {
            if (strcmp(buf2, "/users") == 0) {
                resp_users(buf2);
                send(user->sock, buf2, strlen(buf2), 0);
            } else if (strcmp(buf2, "/dm") == 0) {
                char *id = strtok(NULL, " ");
                char *msg = strtok(NULL, " ");
                char *result = buf + (msg - buf2);

                for (int i = 0; i < client_num; i++) {
                    if (strcmp(users[i]->name, id) == 0) {
                        snprintf(buf2, 255, "[%s] %s\n", user->name, result);
                        send(users[i]->sock, buf2, strlen(buf2), 0);
                        break;
                    }
                }
            } else if (strcmp(buf2, "/start") == 0) {
                char *id = strtok(NULL, " ");

                for (int i = 0; i < client_num; i++) {
                    if (strcmp(users[i]->name, id) == 0) {
                        create_room(user, users[i]);
                        break;
                    }
                }
            } else if (strcmp(buf2, "/new") == 0) {
                // /new 방이름
                char *room_name = strtok(NULL, "\n");
                if (!room_name) {
                    send(user->sock, "Usage: /new <chatroom name>\n", 29, 0);
                } else {
                    int no = create_chatroom(room_name);
                    if (no < 0) {
                        send(user->sock, "Cannot create more chatrooms.\n", 30, 0);
                    } else {
                        char msg[128];
                        snprintf(msg, sizeof(msg),
                                 "Chatroom '%s' created (no: %d).\n",
                                 room_name, no);
                        send(user->sock, msg, strlen(msg), 0);
                    }
                }
            } else if (strcmp(buf2, "/chats") == 0) {
                // /chats
                char listbuf[512];
                resp_chatrooms(listbuf);
                send(user->sock, listbuf, strlen(listbuf), 0);
            } else if (strcmp(buf2, "/enter") == 0) {
                // /enter 번호
                char *num_str = strtok(NULL, " ");
                int no = num_str ? atoi(num_str) : 0;
                if (no <= 0 || no > chatroom_count) {
                    send(user->sock, "Invalid chatroom number.\n", 25, 0);
                } else {
                    user->chat_room = no;
                    char msg[64];
                    snprintf(msg, sizeof(msg),
                             "Entered chatroom '%s'.\n",
                             chatrooms[no-1].name);
                    send(user->sock, msg, strlen(msg), 0);
                }
            } else if (strcmp(buf2, "/exit") == 0) {
                // /exit
                user->chat_room = ROOM0;
                send(user->sock, "Exited chatroom.\n", 18, 0);
            } else if (user->chat_room != ROOM0) {
                snprintf(buf2, 255, "[%s] %s\n", user->name, buf);
                // 1) 공용 채팅방인지 확인
                int cr_idx = -1;
                for (int i = 0; i < chatroom_count; i++) {
                    if (chatrooms[i].no == user->chat_room) {
                        cr_idx = i;
                        break;
                    }
                }
                if (cr_idx >= 0) {
                    // 공용 채팅방: 같은 숫자의 chat_room에 들어있는 모든 유저에게
                    for (int i = 0; i < client_num; i++) {
                        if (users[i]->is_conn && users[i]->chat_room == user->chat_room) {
                            send(users[i]->sock, buf2, strlen(buf2), 0);
                        }
                    }
                } else {
                    // 비밀 채팅방: rooms[] 배열 사용 (원래 방식)
                    int pr_idx = user->chat_room - 1;
                    if (pr_idx >= 0 && pr_idx < room_num) {
                        for (int i = 0; i < 2; i++) {
                            userinfo_t *peer = rooms[pr_idx].member[i];
                            if (peer && peer->is_conn) {
                                send(peer->sock, buf2, strlen(buf2), 0);
                            }
                        }
                    }
                }
            } else {
            /* general message */
            snprintf(buf2, 255, "[%s] %s\n", user->name, buf);

            for (int i = 0; i < client_num; i++) {
                if (users[i]->is_conn) {
                    send(users[i]->sock, buf2, strlen(buf2), 0);
                }
                }
            }
        }
    }
}

int main() {
    char buf[256];
    struct sockaddr_in sin, cli;
    int sd, ns, clientlen = sizeof(cli);
    pthread_t *client_thread;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    int optval = 1;
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = 0;
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin))) {
        perror("bind");
        exit(1);
    }

    if (listen(sd, 5)) {
        perror("listen");
        exit(1);
    }

    int epfd = epoll_create(1);
    struct epoll_event ev, clients[10];
    int epoll_num = 0;

    /* socket */
    ev.events = EPOLLIN;
    ev.data.fd = sd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);

    /* stdin */
    ev.events = EPOLLIN;
    ev.data.fd = 0;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);

    printf("> ");
    fflush(stdout);

    while (1) {
        if ((epoll_num = epoll_wait(epfd, clients, 10, -1)) > 0) {
            for (int i = 0; i < epoll_num; i++) {
                if (clients[i].data.fd == sd) {
                    userinfo_t *user;
                    ns = accept(sd, (struct sockaddr *)&cli, &clientlen);
                    //ev.events = EPOLLIN;
                    //ev.data.fd = ns;
                    //epoll_ctl(epfd, EPOLL_CTL_ADD, ns, &ev);
                    user = (userinfo_t *)malloc(sizeof(userinfo_t));
                    user->sock = ns;
                    user->is_conn = 1;
                    user->chat_room = ROOM0;
                    users[client_num] = user;
                    client_num++;
                    pthread_create(&(user->thread), NULL, client_process, user);
                } else {
                    char command[64];

                    /* command process */
                    fgets(command, 63, stdin);
                    strtok(command, " \n");
                    if (strcmp(command, "users") == 0) {
                        /* show user infos */
                        show_userinfo();
                    } else if (strcmp(command, "chats") == 0) {
                        show_chatrooms();
                    } else if (strcmp(command, "") == 0) {
                    }
                    printf("> ");
                    fflush(stdout);
                }
            }
        }
    }
    close(sd);
}
