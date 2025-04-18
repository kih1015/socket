#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

#define SERVERPORT 23  // 텔넷 기본 포트
#define BUFSIZE 512
#define MAX_SESSIONS 10

typedef struct {
    int sock;
    char username[32];
    int authenticated;
} Session;

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

void set_nonblocking_mode(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

void handle_telnet_command(Session *session, char *buf, int len) {
    if (!session->authenticated) {
        if (strncmp(buf, "login: ", 7) == 0) {
            strncpy(session->username, buf + 7, sizeof(session->username) - 1);
            session->username[strcspn(session->username, "\r\n")] = 0;
            char *msg = "Password: ";
            send(session->sock, msg, strlen(msg), 0);
        }
        else if (strncmp(buf, "password: ", 10) == 0) {
            // 간단한 인증 (실제로는 더 안전한 방식 사용해야 함)
            if (strncmp(buf + 10, "1234\r\n", 6) == 0) {
                session->authenticated = 1;
                char *msg = "\r\nWelcome to Telnet Server!\r\n$ ";
                send(session->sock, msg, strlen(msg), 0);
            }
            else {
                char *msg = "\r\nLogin failed!\r\nlogin: ";
                send(session->sock, msg, strlen(msg), 0);
            }
        }
        return;
    }

    // 명령어 처리
    buf[strcspn(buf, "\r\n")] = 0;
    if (strcmp(buf, "exit") == 0) {
        char *msg = "Goodbye!\r\n";
        send(session->sock, msg, strlen(msg), 0);
        close(session->sock);
        session->sock = -1;
    }
    else if (strcmp(buf, "help") == 0) {
        char *msg = "Available commands:\r\n"
                   "  help - show this help\r\n"
                   "  exit - disconnect\r\n"
                   "$ ";
        send(session->sock, msg, strlen(msg), 0);
    }
    else {
        char msg[BUFSIZE];
        snprintf(msg, sizeof(msg), "Unknown command: %s\r\n$ ", buf);
        send(session->sock, msg, strlen(msg), 0);
    }
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;
    Session sessions[MAX_SESSIONS] = {0};
    char buf[BUFSIZE];
    
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) err_quit("socket()");
    
    set_nonblocking_mode(server_sock);
    
    int optval = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVERPORT);
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        err_quit("bind()");
    
    if (listen(server_sock, SOMAXCONN) == -1)
        err_quit("listen()");
    
    printf("텔넷 서버 시작...\n");
    
    fd_set readfds;
    int maxfd;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        maxfd = server_sock;
        
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (sessions[i].sock > 0) {
                FD_SET(sessions[i].sock, &readfds);
                if (sessions[i].sock > maxfd)
                    maxfd = sessions[i].sock;
            }
        }
        
        struct timeval tv = {0, 10000};
        int ready = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        
        if (ready == -1) {
            if (errno == EINTR) continue;
            err_quit("select()");
        }
        
        if (FD_ISSET(server_sock, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
            
            if (client_sock != -1) {
                set_nonblocking_mode(client_sock);
                
                int i;
                for (i = 0; i < MAX_SESSIONS; i++) {
                    if (sessions[i].sock <= 0) {
                        sessions[i].sock = client_sock;
                        sessions[i].authenticated = 0;
                        break;
                    }
                }
                
                if (i < MAX_SESSIONS) {
                    printf("새 클라이언트 연결: %d\n", client_sock);
                    char *msg = "Welcome to Telnet Server\r\nlogin: ";
                    send(client_sock, msg, strlen(msg), 0);
                }
                else {
                    printf("최대 세션 수 초과\n");
                    close(client_sock);
                }
            }
        }
        
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (sessions[i].sock > 0 && FD_ISSET(sessions[i].sock, &readfds)) {
                memset(buf, 0, BUFSIZE);
                int nbytes = recv(sessions[i].sock, buf, BUFSIZE - 1, 0);
                
                if (nbytes <= 0) {
                    if (nbytes == 0 || errno != EWOULDBLOCK) {
                        printf("클라이언트 종료: %d\n", sessions[i].sock);
                        close(sessions[i].sock);
                        sessions[i].sock = -1;
                    }
                }
                else {
                    handle_telnet_command(&sessions[i], buf, nbytes);
                }
            }
        }
    }
    
    close(server_sock);
    return 0;
} 