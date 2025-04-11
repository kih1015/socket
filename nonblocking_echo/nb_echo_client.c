#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE 512

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buf[BUFSIZE];
    
    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");
    
    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVERIP);
    server_addr.sin_port = htons(SERVERPORT);
    
    // 서버 연결
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        err_quit("connect()");
    
    printf("서버 연결 완료\n");
    printf("메시지를 입력하세요 (종료하려면 빈 줄 입력):\n");
    
    while (1) {
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(sock, &readset);
        FD_SET(STDIN_FILENO, &readset);
        
        int maxfd = (sock > STDIN_FILENO ? sock : STDIN_FILENO);
        struct timeval tv = {1, 0}; // 1초 타임아웃
        
        int ready = select(maxfd + 1, &readset, NULL, NULL, &tv);
        if (ready == -1) {
            err_quit("select()");
        }
        
        // 서버로부터 데이터 수신
        if (FD_ISSET(sock, &readset)) {
            memset(buf, 0, BUFSIZE);
            int received = recv(sock, buf, BUFSIZE - 1, 0);
            
            if (received <= 0) {
                if (received == 0) {
                    printf("\n서버 연결 종료\n");
                } else {
                    printf("\n서버 수신 오류\n");
                }
                break;
            }
            
            buf[received] = '\0';
            printf("[에코 메시지] %s\n", buf);
        }
        
        // 키보드 입력 처리
        if (FD_ISSET(STDIN_FILENO, &readset)) {
            if (fgets(buf, BUFSIZE, stdin) == NULL)
                break;
            
            // 문자열 끝의 개행 문자 제거
            int len = strlen(buf);
            if (buf[len-1] == '\n')
                buf[len-1] = '\0';
            
            if (strlen(buf) == 0)
                break;
            
            // 데이터 전송
            if (send(sock, buf, strlen(buf), 0) == -1) {
                printf("\n서버 전송 오류\n");
                break;
            }
        }
    }
    
    close(sock);
    return 0;
} 