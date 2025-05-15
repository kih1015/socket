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

int create_chatroom(const char *name)
{
    if (chatroom_count >= MAX_CHATROOMS) return -1;
    chatrooms[chatroom_count].no = chatroom_count + 1;
    strncpy(chatrooms[chatroom_count].name, name, 63);
    chatrooms[chatroom_count].name[63] = '\0';
    chatroom_count++;
    return chatroom_count;
}

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
    int rb;

    snprintf(buf, 255, "Welcome!\n Input user name: ");
    send(user->sock, buf, strlen(buf), 0);
    rb = recv(user->sock, buf, 63, 0);
    buf[rb] = '\0';
    strtok(buf, "\n");
    strncpy(user->name, buf, 63);
    user->is_conn = 1;
    user->chat_room = ROOM0;

    while (1) {
        rb = recv(user->sock, buf, sizeof(buf) - 1, 0);
        if (rb <= 0) {
            user->is_conn = 0;
            printf("user %s disconnected\n", user->name);
            pthread_exit(NULL);
        }
        buf[rb] = '\0';
        strtok(buf, "\n");
        strcpy(buf2, buf);
        char *cmd = strtok(buf2, " ");

        if (buf[0] == '/') {
            if (strcmp(cmd, "/users") == 0) {
                resp_users(buf2);
                send(user->sock, buf2, strlen(buf2), 0);
            } else if (strcmp(cmd, "/dm") == 0) {
                char *id = strtok(NULL, " ");
                char *text = strtok(NULL, "");
                if (!id || !text) {
                    send(user->sock, "Usage: /dm <user> <message>\n", 30, 0);
                    continue;
                }
                char out[256];
                snprintf(out, sizeof(out), "[DM %s->%s] %s\n", user->name, id, text);
                int found = 0;
                for (int i = 0; i < client_num; i++) {
                    if (users[i]->is_conn && strcmp(users[i]->name, id) == 0) {
                        send(users[i]->sock, out, strlen(out), 0);
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    send(user->sock, "User not found or disconnected.\n", 33, 0);
                }
            } else if (strcmp(cmd, "/start") == 0) {
                char *id = strtok(NULL, " ");
                if (!id) continue;
                for (int i = 0; i < client_num; i++) {
                    if (strcmp(users[i]->name, id) == 0) {
                        create_room(user, users[i]);
                        break;
                    }
                }
            } else if (strcmp(cmd, "/new") == 0) {
                char *room_name = strtok(NULL, "\n");
                if (!room_name) {
                    send(user->sock, "Usage: /new <chatroom name>\n", 29, 0);
                } else {
                    int no = create_chatroom(room_name);
                    if (no < 0) {
                        send(user->sock, "Cannot create more chatrooms.\n", 30, 0);
                    } else {
                        char msg[128];
                        snprintf(msg, sizeof(msg), "Chatroom '%s' created (no: %d).\n", room_name, no);
                        send(user->sock, msg, strlen(msg), 0);
                    }
                }
            } else if (strcmp(cmd, "/chats") == 0) {
                char listbuf[512];
                resp_chatrooms(listbuf);
                send(user->sock, listbuf, strlen(listbuf), 0);
            } else if (strcmp(cmd, "/enter") == 0) {
                char *num_str = strtok(NULL, " ");
                int no = num_str ? atoi(num_str) : 0;
                if (no <= 0 || no > chatroom_count) {
                    send(user->sock, "Invalid chatroom number.\n", 25, 0);
                } else {
                    user->chat_room = no;
                    char msg[64];
                    snprintf(msg, sizeof(msg), "Entered chatroom '%s'.\n",
                             chatrooms[no-1].name);
                    send(user->sock, msg, strlen(msg), 0);
                }
            } else if (strcmp(cmd, "/exit") == 0) {
                user->chat_room = ROOM0;
                send(user->sock, "Exited chatroom.\n", 18, 0);
            }
        }
        else if (user->chat_room != ROOM0) {
            snprintf(buf2, 255, "[%s] %s\n", user->name, buf);
            int cr_idx = -1;
            for (int i = 0; i < chatroom_count; i++) {
                if (chatrooms[i].no == user->chat_room) {
                    cr_idx = i;
                    break;
                }
            }
            if (cr_idx >= 0) {
                for (int i = 0; i < client_num; i++) {
                    if (users[i]->is_conn && users[i]->chat_room == user->chat_room) {
                        send(users[i]->sock, buf2, strlen(buf2), 0);
                    }
                }
            } else {
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
        }
        else {
            /* lobby message: ROOM0 참여자만 */
            snprintf(buf2, 255, "[%s] %s\n", user->name, buf);
            for (int i = 0; i < client_num; i++) {
                if (users[i]->is_conn && users[i]->chat_room == ROOM0) {
                    send(users[i]->sock, buf2, strlen(buf2), 0);
                }
            }
        }
    }
}

int main() {
    char buf[256];
    struct sockaddr_in sin, cli;
    int sd, ns, clientlen = sizeof(cli);

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

    ev.events = EPOLLIN;
    ev.data.fd = sd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sd, &ev);

    ev.events = EPOLLIN;
    ev.data.fd = 0;
    epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &ev);

    printf("> "); fflush(stdout);

    while (1) {
        if ((epoll_num = epoll_wait(epfd, clients, 10, -1)) > 0) {
            for (int i = 0; i < epoll_num; i++) {
                if (clients[i].data.fd == sd) {
                    ns = accept(sd, (struct sockaddr *)&cli, &clientlen);
                    userinfo_t *user = calloc(1, sizeof(userinfo_t));
                    user->sock = ns;
                    user->is_conn = 1;
                    user->chat_room = ROOM0;
                    users[client_num++] = user;
                    pthread_create(&user->thread, NULL, client_process, user);
                } else {
                    char command[64];
                    fgets(command, sizeof(command), stdin);
                    strtok(command, " \n");
                    if (strcmp(command, "users") == 0)    show_userinfo();
                    else if (strcmp(command, "chats") == 0) show_chatrooms();
                    printf("> "); fflush(stdout);
                }
            }
        }
    }
    close(sd);
}
