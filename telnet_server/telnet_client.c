#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

#define SERVERIP "127.0.0.1"
#define SERVERPORT 23
#define BUFSIZE 512

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buf[BUFSIZE];
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVERIP);
    server_addr.sin_port = htons(SERVERPORT);
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        err_quit("connect()");
    
    fd_set readfds;
    int maxfd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);
        
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1)
            err_quit("select()");
        
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buf, BUFSIZE, stdin) == NULL)
                break;
            
            if (send(sock, buf, strlen(buf), 0) == -1)
                err_quit("send()");
        }
        
        if (FD_ISSET(sock, &readfds)) {
            int nbytes = recv(sock, buf, BUFSIZE - 1, 0);
            if (nbytes <= 0) {
                if (nbytes == 0)
                    printf("서버 연결 종료\n");
                else
                    perror("recv()");
                break;
            }
            
            buf[nbytes] = '\0';
            printf("%s", buf);
            fflush(stdout);
        }
    }
    
    close(sock);
    return 0;
} 