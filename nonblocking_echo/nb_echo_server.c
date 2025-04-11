#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

#define SERVERPORT 9000
#define BUFSIZE 512
#define TIMEOUT_SECONDS 30

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char buf[BUFSIZE];
    int retval;
    
    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) err_quit("socket()");
    
    // SO_REUSEADDR 소켓 옵션 설정
    int optval = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (retval == -1) err_quit("setsockopt()");
    
    // bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVERPORT);
    retval = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (retval == -1) err_quit("bind()");
    
    // listen
    retval = listen(server_sock, SOMAXCONN);
    if (retval == -1) err_quit("listen()");
    
    printf("서버 시작...\n");
    
    while (1) {
        // accept()
        socklen_t client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
        if (client_sock == -1) {
            err_quit("accept()");
            continue;
        }
        
        // 클라이언트 접속 정보 출력
        printf("\n[TCP 서버] 클라이언트 접속: IP=%s, Port=%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // 수신 타임아웃 설정
        struct timeval tv;
        tv.tv_sec = TIMEOUT_SECONDS;
        tv.tv_usec = 0;
        retval = setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (retval == -1) err_quit("setsockopt()");
        
        // 클라이언트와 데이터 통신
        while (1) {
            // 데이터 받기
            memset(buf, 0, BUFSIZE);
            retval = recv(client_sock, buf, BUFSIZE, 0);
            if (retval == -1) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    printf("클라이언트 타임아웃으로 연결 종료\n");
                } else {
                    printf("recv() 오류\n");
                }
                break;
            } else if (retval == 0) {
                printf("클라이언트 정상 종료\n");
                break;
            }
            
            // 받은 데이터 출력
            printf("[TCP/%s:%d] %s\n", inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port), buf);
            
            // 데이터 보내기
            retval = send(client_sock, buf, retval, 0);
            if (retval == -1) {
                printf("send() 오류\n");
                break;
            }
        }
        
        // 소켓 닫기
        close(client_sock);
        printf("[TCP 서버] 클라이언트 종료: IP=%s, Port=%d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }
    
    // 서버 소켓 닫기
    close(server_sock);
    return 0;
} 