#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PORTNUM 9000
#define BUFFER_SIZE 256

int main() {
    char buf[BUFFER_SIZE];
    struct sockaddr_in sin, cli;
    int sd, ns, clientlen = sizeof(cli);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = INADDR_ANY;  // 모든 인터페이스에서 연결 수락

    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin))) {
        perror("bind");
        exit(1);
    }

    if (listen(sd, 5)) {
        perror("listen");
        exit(1);
    }

    printf("에코 서버가 시작되었습니다. 포트: %d\n", PORTNUM);
    
    if ((ns = accept(sd, (struct sockaddr *)&cli, &clientlen)) == -1) {
        perror("accept");
        exit(1);
    }

    printf("클라이언트 연결됨: %s\n", inet_ntoa(cli.sin_addr));
    
    // 클라이언트로부터 메시지를 받아서 그대로 다시 보내는 에코 서버
    while (1) {
        memset(buf, 0, BUFFER_SIZE);
        int recv_len = recv(ns, buf, BUFFER_SIZE, 0);
        
        if (recv_len <= 0) {
            printf("클라이언트 연결 종료\n");
            break;
        }
        
        printf("수신 메시지: %s\n", buf);
        
        // 받은 메시지를 그대로 다시 전송
        if (send(ns, buf, recv_len, 0) == -1) {
            perror("send");
            break;
        }
        
        printf("메시지 에코 완료\n");
    }
    
    close(ns);
    close(sd);
    return 0;
} 
