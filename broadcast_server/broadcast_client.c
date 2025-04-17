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
    int retval;
    
    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");
    
    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVERIP);
    server_addr.sin_port = htons(SERVERPORT);
    
    // 서버 연결
    retval = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (retval == -1) err_quit("connect()");
    
    printf("서버 연결 완료\n");
    printf("채팅을 시작합니다.\n");
    printf("메시지를 입력하세요 (종료하려면 빈 줄 입력):\n");
    
    while (1) {
        // select() 준비
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(0, &readset);     // 표준 입력
        FD_SET(sock, &readset);  // 소켓
        
        // select() 호출
        retval = select(sock + 1, &readset, NULL, NULL, NULL);
        if (retval == -1) {
            err_quit("select()");
            break;
        }
        
        // 소켓 수신 처리
        if (FD_ISSET(sock, &readset)) {
            memset(buf, 0, BUFSIZE);
            retval = recv(sock, buf, BUFSIZE - 1, 0);
            if (retval == -1) {
                err_quit("recv()");
                break;
            } else if (retval == 0) {
                printf("\n서버 연결 종료\n");
                break;
            }
            
            // 받은 데이터 출력
            printf("\n[메시지] %s\n", buf);
            printf("메시지 입력: ");
            fflush(stdout);
        }
        
        // 표준 입력 처리
        if (FD_ISSET(0, &readset)) {
            if (fgets(buf, BUFSIZE, stdin) == NULL)
                break;
            
            // 문자열 끝의 개행 문자 제거
            int len = strlen(buf);
            if (buf[len-1] == '\n')
                buf[len-1] = '\0';
            
            if (strlen(buf) == 0)
                break;
            
            // 데이터 보내기
            retval = send(sock, buf, strlen(buf), 0);
            if (retval == -1) {
                err_quit("send()");
                break;
            }
        }
    }
    
    // 소켓 닫기
    close(sock);
    return 0;
} 