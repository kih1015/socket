#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#define SERVERPORT 9000
#define BUFSIZE 512

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;
    
    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) err_quit("socket()");
    
    // SO_REUSEADDR 설정
    int optval = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        err_quit("setsockopt()");
    
    // Non-blocking 설정
    int flags = fcntl(server_sock, F_GETFL, 0);
    fcntl(server_sock, F_SETFL, flags | O_NONBLOCK);
    
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
    
    // 클라이언트 정보
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    int client_sock;
    char buf[BUFSIZE];
    
    printf("서버 시작...\n");
    
    while (1) {
        // 클라이언트 접속 수락
        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
        
        if (client_sock == -1) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                usleep(10000); // CPU 사용률 감소
                continue;
            }
            err_quit("accept()");
        }
        
        // 접속한 클라이언트 정보 출력
        printf("\n[TCP 서버] 클라이언트 접속: IP=%s, Port=%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // 수신 타임아웃 설정 (30초)
        struct timeval timeout;
        timeout.tv_sec = 30;
        timeout.tv_usec = 0;
        if (setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
            err_quit("setsockopt()");
        
        // Non-blocking 설정
        flags = fcntl(client_sock, F_GETFL, 0);
        fcntl(client_sock, F_SETFL, flags | O_NONBLOCK);
        
        // 데이터 수신 및 에코
        while (1) {
            memset(buf, 0, BUFSIZE);
            int received = recv(client_sock, buf, BUFSIZE, 0);
            
            if (received < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    usleep(10000);
                    continue;
                } else if (errno == ETIMEDOUT) {
                    printf("클라이언트 타임아웃으로 연결 종료\n");
                    break;
                } else {
                    err_quit("recv()");
                }
            } else if (received == 0) {
                printf("클라이언트 연결 종료\n");
                break;
            }
            
            // 받은 데이터 출력
            printf("[TCP/%s:%d] %s\n", inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port), buf);
            
            // 데이터 에코
            if (send(client_sock, buf, received, 0) == -1)
                err_quit("send()");
        }
        
        close(client_sock);
    }
    
    close(server_sock);
    return 0;
} 