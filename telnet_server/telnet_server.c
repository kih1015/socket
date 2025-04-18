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

// telnet 프로토콜 명령어
#define IAC     255  // 0xFF: Interpret As Command
#define WILL    251  // 0xFB: 옵션 활성화 요청
#define WONT    252  // 0xFC: 옵션 비활성화 통보
#define DO      253  // 0xFD: 옵션 활성화 요청
#define DONT    254  // 0xFE: 옵션 비활성화 요청

// 상태 머신 상태
enum {
    STATE_DATA,      // 일반 데이터
    STATE_IAC,       // IAC 수신
    STATE_OPTION     // 옵션 수신
};

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

// 터미널 설정을 위한 전역 변수
struct termios orig_termios;

// RAW 모드 설정
void set_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    
    // 입력 모드 플래그
    raw.c_iflag &= ~(ICRNL | IXON);
    
    // 출력 모드 플래그
    raw.c_oflag &= ~(OPOST);
    
    // 로컬 모드 플래그
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    
    // 제어 문자
    raw.c_cc[VMIN] = 1;  // 최소 1바이트 입력
    raw.c_cc[VTIME] = 0; // 타임아웃 없음
    
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// 원래 모드로 복구
void restore_terminal() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// IAC 명령에 대한 응답 전송
void send_telnet_response(int sock, unsigned char cmd, unsigned char option) {
    unsigned char response[3];
    response[0] = IAC;
    
    if (cmd == DO)
        response[1] = WONT;    // DO에 대해 WONT로 응답
    else if (cmd == WILL)
        response[1] = DONT;    // WILL에 대해 DONT로 응답
    else
        return;                // 다른 명령은 무시
        
    response[2] = option;
    write(sock, response, 3);
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;
    int client_socks[MAX_CLIENTS] = {0};
    char buf[BUFSIZE];
    
    // RAW 모드 설정
    set_raw_mode();
    atexit(restore_terminal);
    
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) err_quit("socket()");
    
    int optval = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVERPORT);
    
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        err_quit("bind()");
    
    if (listen(server_sock, SOMAXCONN) == -1)
        err_quit("listen()");
    
    printf("텔넷 서버 시작...\n");
    
    fd_set readfds;
    int maxfd;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_sock, &readfds);
        maxfd = server_sock;
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_socks[i] > 0) {
                FD_SET(client_socks[i], &readfds);
                if (client_socks[i] > maxfd)
                    maxfd = client_socks[i];
            }
        }
        
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) continue;
            err_quit("select()");
        }
        
        if (FD_ISSET(server_sock, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
            
            if (client_sock == -1) {
                printf("accept() 오류\n");
                continue;
            }
            
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
    
    close(server_sock);
    return 0;
} 