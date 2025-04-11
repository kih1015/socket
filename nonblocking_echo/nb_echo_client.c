#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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
    
    // 데이터 통신
    while (1) {
        // 데이터 입력
        printf("\n[보낼 데이터] ");
        if (fgets(buf, BUFSIZE, stdin) == NULL)
            break;
        
        // 문자열 끝의 개행 문자 제거
        int len = strlen(buf);
        if (buf[len-1] == '\n')
            buf[len-1] = '\0';
        
        if (strlen(buf) == 0)
            break;
        
        // 데이터 전송
        if (send(sock, buf, strlen(buf), 0) == -1)
            err_quit("send()");
        
        // 데이터 수신
        int received = recv(sock, buf, BUFSIZE-1, 0);
        if (received == -1)
            err_quit("recv()");
        else if (received == 0)
            break;
        
        // 받은 데이터 출력
        buf[received] = '\0';
        printf("[에코 메시지] %s\n", buf);
    }
    
    close(sock);
    return 0;
} 