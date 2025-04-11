#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

#define SERVERPORT 9000
#define BUFSIZE 512
#define TIMEOUT_SECONDS 30
#define MAX_CLIENTS 10

typedef struct {
    int sock;
    struct sockaddr_in addr;
    time_t last_recv_time;
    char ip[16];
    int port;
} ClientInfo;

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;
    ClientInfo clients[MAX_CLIENTS] = {0};
    int client_count = 0;
    
    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) err_quit("socket()");
    
    // SO_REUSEADDR 설정
    int optval = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        err_quit("setsockopt()");
    
    // bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVERPORT);
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        err_quit("bind()");
    
    // listen
    if (listen(server_sock, SOMAXCONN) == -1)
        err_quit("listen()");
    
    printf("서버 시작...\n");
    
    while (1) {
        fd_set readset;
        struct timeval tv = {1, 0}; // 1초 타임아웃
        int maxfd = server_sock;
        
        FD_ZERO(&readset);
        FD_SET(server_sock, &readset);
        
        // 현재 연결된 모든 클라이언트 소켓을 fd_set에 추가
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0) {
                FD_SET(clients[i].sock, &readset);
                if (clients[i].sock > maxfd)
                    maxfd = clients[i].sock;
            }
        }
        
        // select로 입출력 가능한 소켓 감시
        int ready = select(maxfd + 1, &readset, NULL, NULL, &tv);
        if (ready == -1) {
            err_quit("select()");
        }
        
        // 새로운 클라이언트 연결 체크
        if (FD_ISSET(server_sock, &readset)) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
            
            if (client_sock == -1) {
                err_quit("accept()");
            }
            
            // 새 클라이언트 정보 저장
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == 0) {
                    clients[i].sock = client_sock;
                    clients[i].addr = client_addr;
                    clients[i].last_recv_time = time(NULL);
                    strcpy(clients[i].ip, inet_ntoa(client_addr.sin_addr));
                    clients[i].port = ntohs(client_addr.sin_port);
                    client_count++;
                    printf("\n[TCP 서버] 클라이언트 접속: IP=%s, Port=%d\n",
                           clients[i].ip, clients[i].port);
                    break;
                }
            }
        }
        
        // 각 클라이언트의 데이터 수신 체크 및 타임아웃 처리
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0) {
                // 타임아웃 체크
                time_t current_time = time(NULL);
                if (current_time - clients[i].last_recv_time >= TIMEOUT_SECONDS) {
                    printf("클라이언트 타임아웃으로 연결 종료 (IP=%s, Port=%d)\n",
                           clients[i].ip, clients[i].port);
                    close(clients[i].sock);
                    clients[i].sock = 0;
                    client_count--;
                    continue;
                }
                
                // 데이터 수신 체크
                if (FD_ISSET(clients[i].sock, &readset)) {
                    char buf[BUFSIZE];
                    memset(buf, 0, BUFSIZE);
                    int received = recv(clients[i].sock, buf, BUFSIZE, 0);
                    
                    if (received <= 0) {
                        if (received == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                            printf("클라이언트 연결 종료 (IP=%s, Port=%d)\n",
                                   clients[i].ip, clients[i].port);
                            close(clients[i].sock);
                            clients[i].sock = 0;
                            client_count--;
                            continue;
                        }
                    } else {
                        // 데이터 수신 성공
                        clients[i].last_recv_time = time(NULL);
                        printf("[TCP/%s:%d] %s\n", clients[i].ip, clients[i].port, buf);
                        
                        // 에코
                        if (send(clients[i].sock, buf, received, 0) == -1) {
                            printf("send() 실패 (IP=%s, Port=%d)\n",
                                   clients[i].ip, clients[i].port);
                            close(clients[i].sock);
                            clients[i].sock = 0;
                            client_count--;
                        }
                    }
                }
            }
        }
    }
    
    close(server_sock);
    return 0;
} 