#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PORTNUM 9000
#define BUFFER_SIZE 256

int main() {
    int sd;
    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];
    struct sockaddr_in sin;

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");  // localhost IP

    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin))) {
        perror("connect");
        exit(1);
    }

    printf("서버에 연결되었습니다.\n");
    
    while (1) {
        printf("전송할 메시지 (종료하려면 'exit' 입력): ");
        fgets(send_buf, BUFFER_SIZE, stdin);
        
        // 개행 문자 제거
        send_buf[strcspn(send_buf, "\n")] = '\0';
        
        // 'exit' 입력 시 종료
        if (strcmp(send_buf, "exit") == 0) {
            printf("프로그램을 종료합니다.\n");
            break;
        }
        
        // 메시지 전송
        if (send(sd, send_buf, strlen(send_buf), 0) == -1) {
            perror("send");
            break;
        }
        
        // 서버로부터 에코된 메시지 수신
        memset(recv_buf, 0, BUFFER_SIZE);
        if (recv(sd, recv_buf, BUFFER_SIZE, 0) == -1) {
            perror("recv");
            break;
        }
        
        printf("서버로부터 에코: %s\n", recv_buf);
    }
    
    close(sd);
    return 0;
} 
