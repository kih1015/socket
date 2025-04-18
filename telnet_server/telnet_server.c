#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <termios.h>

#define SERVERPORT 23
#define BUFSIZE 512
#define MAX_CLIENTS 10

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

// 터미널 설정을 위한 전역 변수
struct termios orig_termios;

// 터미널 설정 초기화
void init_terminal() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}

// 터미널 설정 복원
void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;
    int client_socks[MAX_CLIENTS] = {0};
    char buf[BUFSIZE];
    
    // 터미널 설정
    init_terminal();
    
    // 프로그램 종료 시 터미널 설정 복원
    atexit(restore_terminal);
    
    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) err_quit("socket()");
    
    // SO_REUSEADDR 옵션 설정
    int optval = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    // bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVERPORT);
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        err_quit("bind()");
    
    // listen
    if (listen(server_sock, SOMAXCONN) == -1)
        err_quit("listen()");
    
    printf("텔넷 서버 시작...\n");
    
    fd_set readfds;
    int maxfd;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        maxfd = server_sock;
        
        // 클라이언트 소켓 설정
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socks[i] > 0) {
                FD_SET(client_socks[i], &readfds);
                if (client_socks[i] > maxfd)
                    maxfd = client_socks[i];
            }
        }
        
        // select() 호출
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) continue;
            err_quit("select()");
        }
        
        // 새 클라이언트 연결 처리
        if (FD_ISSET(server_sock, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
            
            if (client_sock == -1) {
                printf("accept() 오류\n");
                continue;
            }
            
            // 새 클라이언트 소켓 저장
            int i;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_socks[i] == 0) {
                    client_socks[i] = client_sock;
                    printf("새 클라이언트 연결: %d\n", client_sock);
                    break;
                }
            }
            
            if (i == MAX_CLIENTS) {
                printf("최대 클라이언트 수 초과\n");
                close(client_sock);
            }
        }
        
        // 클라이언트 데이터 수신 처리
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socks[i] > 0 && FD_ISSET(client_socks[i], &readfds)) {
                memset(buf, 0, BUFSIZE);
                int nbytes = recv(client_socks[i], buf, BUFSIZE - 1, 0);
                
                if (nbytes <= 0) {
                    printf("클라이언트 종료: %d\n", client_socks[i]);
                    close(client_socks[i]);
                    client_socks[i] = 0;
                }
                else {
                    // 받은 데이터를 모든 클라이언트에게 전송
                    printf("[Client %d] %s", client_socks[i], buf);
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (client_socks[j] > 0) {
                            send(client_socks[j], buf, nbytes, 0);
                        }
                    }
                }
            }
        }
    }
    
    // 서버 종료
    close(server_sock);
    return 0;
} 