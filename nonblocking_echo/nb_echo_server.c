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

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int server_sock, client_sock = -1;
    struct sockaddr_in server_addr, client_addr;
    char buf[BUFSIZE];
    time_t last_recv_time;
    
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
        
        FD_ZERO(&readset);
        FD_SET(server_sock, &readset);
        
        int maxfd = server_sock;
        
        // 클라이언트가 연결되어 있으면 해당 소켓도 감시
        if (client_sock != -1) {
            FD_SET(client_sock, &readset);
            if (client_sock > maxfd)
                maxfd = client_sock;
        }
        
        // select로 입출력 가능한 소켓 감시
        int ready = select(maxfd + 1, &readset, NULL, NULL, &tv);
        if (ready == -1) {
            err_quit("select()");
        }
        
        // 새로운 클라이언트 연결 체크
        if (FD_ISSET(server_sock, &readset)) {
            if (client_sock == -1) {  // 현재 연결된 클라이언트가 없을 때만 새 연결 수락
                socklen_t client_addr_size = sizeof(client_addr);
                client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
                
                if (client_sock == -1) {
                    err_quit("accept()");
                }
                
                last_recv_time = time(NULL);
                printf("\n[TCP 서버] 클라이언트 접속: IP=%s, Port=%d\n",
                       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            }
        }
        
        // 연결된 클라이언트 처리
        if (client_sock != -1 && FD_ISSET(client_sock, &readset)) {
            // 타임아웃 체크
            time_t current_time = time(NULL);
            if (current_time - last_recv_time >= TIMEOUT_SECONDS) {
                printf("클라이언트 타임아웃으로 연결 종료\n");
                close(client_sock);
                client_sock = -1;
                continue;
            }
            
            // 데이터 수신
            memset(buf, 0, BUFSIZE);
            int received = recv(client_sock, buf, BUFSIZE, 0);
            
            if (received <= 0) {
                if (received == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
                    printf("클라이언트 연결 종료\n");
                    close(client_sock);
                    client_sock = -1;
                    continue;
                }
            } else {
                // 데이터 수신 성공
                last_recv_time = time(NULL);
                printf("[TCP/%s:%d] %s\n", inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port), buf);
                
                // 에코
                if (send(client_sock, buf, received, 0) == -1) {
                    printf("send() 실패\n");
                    close(client_sock);
                    client_sock = -1;
                }
            }
        }
    }
    
    close(server_sock);
    return 0;
} 