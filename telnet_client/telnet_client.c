#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <termios.h>

#define BUFSIZE 512

// 텔넷 프로토콜 명령어
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

struct termios orig_termios;

void set_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

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

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    int sock;
    struct sockaddr_in server_addr;
    unsigned char ch;
    int state = STATE_DATA;    // 초기 상태
    unsigned char telnet_cmd;  // IAC 명령 저장
    
    set_raw_mode();
    atexit(restore_terminal);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) err_quit("socket()");
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        err_quit("connect()");
    
    printf("서버 연결 성공\r\n");
    
    fd_set readfds;
    int maxfd = (sock > STDIN_FILENO) ? sock : STDIN_FILENO;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);
        
        if (select(maxfd + 1, &readfds, NULL, NULL, NULL) == -1)
            err_quit("select()");
        
        // 키보드 입력 처리
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (read(STDIN_FILENO, &ch, 1) != 1) break;
            
            // Ctrl+C 처리
            if (ch == '\x03') {
                write(STDOUT_FILENO, "^C\r\n", 4);
                break;
            }
            
            // 백스페이스 처리
            if (ch == '\x7f') {
                write(STDOUT_FILENO, "\b \b", 3);
                continue;
            }
            
            // 엔터키 처리
            if (ch == '\r') {
                write(STDOUT_FILENO, "\r\n", 2);
                write(sock, "\r\n", 2);
                continue;
            }
            
            // 일반 문자 처리
            write(STDOUT_FILENO, &ch, 1);
            write(sock, &ch, 1);
        }
        
        // 서버로부터 데이터 수신
        if (FD_ISSET(sock, &readfds)) {
            if (read(sock, &ch, 1) <= 0) {
                write(STDOUT_FILENO, "\r\n서버 연결 종료\r\n", 25);
                break;
            }
            
            // 상태 머신으로 텔넷 프로토콜 처리
            switch (state) {
                case STATE_DATA:
                    if (ch == IAC)
                        state = STATE_IAC;
                    else
                        write(STDOUT_FILENO, &ch, 1);
                    break;
                    
                case STATE_IAC:
                    switch (ch) {
                        case WILL:
                        case DO:
                            telnet_cmd = ch;
                            state = STATE_OPTION;
                            break;
                        case IAC:  // IAC IAC는 255 문자를 의미
                            write(STDOUT_FILENO, &ch, 1);
                            state = STATE_DATA;
                            break;
                        default:
                            state = STATE_DATA;
                            break;
                    }
                    break;
                    
                case STATE_OPTION:
                    send_telnet_response(sock, telnet_cmd, ch);
                    state = STATE_DATA;
                    break;
            }
        }
    }
    
    close(sock);
    return 0;
} 