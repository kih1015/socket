#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <termios.h>
#include <netinet/tcp.h>
#include <pthread.h>

#define IAC  255    // 0xFF: Interpret As Command
#define WILL 251    // 0xFB: 옵션 활성화 요청
#define WONT 252    // 0xFC: 옵션 비활성화 통보
#define DO   253    // 0xFD: 옵션 활성화 요청
#define DONT 254    // 0xFE: 옵션 비활성화 요청

enum {
    STATE_DATA,     // 일반 데이터
    STATE_IAC,      // IAC 수신
    STATE_IAC_CMD   // IAC 명령 수신
};

// 전역 변수로 터미널 설정 저장
struct termios org_tios;
int sock;  // 소켓 디스크립터를 전역 변수로 선언

void error_handling(char *message) {
    perror(message);
    exit(1);
}

// 터미널 설정 복원
void restore_terminal() {
    tcsetattr(0, TCSANOW, &org_tios);
}

// SIGINT 핸들러
void sigint_handler(int signo) {
    restore_terminal();  // 터미널 설정 복원
    close(sock);        // 소켓 닫기
    exit(0);
}

// WONT 응답 전송
void sendWONT(int sock, unsigned char option) {
    unsigned char response[3] = {IAC, WONT, option};
    write(sock, response, 3);
}

// DONT 응답 전송
void sendDONT(int sock, unsigned char option) {
    unsigned char response[3] = {IAC, DONT, option};
    write(sock, response, 3);
}

// 수신 스레드 함수
void *receiver_thread(void *arg) {
    int parseState = STATE_DATA;
    unsigned char c;
    unsigned char iacCmd;
    int n;

    while (1) {
        n = read(sock, &c, 1);
        
        if (n < 0) {
            perror("read (server->client)");
            pthread_exit(NULL);
        } else if (n == 0) {
            fprintf(stderr, "\n[INFO] Server closed connection.\n");
            pthread_exit(NULL);
        }

        switch (parseState) {
            case STATE_DATA:
                if (c == IAC) {
                    parseState = STATE_IAC;
                } else {
                    putchar(c);
                    fflush(stdout);
                }
                break;

            case STATE_IAC:
                if (c == IAC) {
                    putchar(IAC);
                    fflush(stdout);
                    parseState = STATE_DATA;
                } else {
                    iacCmd = c;
                    parseState = STATE_IAC_CMD;
                }
                break;

            case STATE_IAC_CMD:
                if (iacCmd == DO) {
                    sendWONT(sock, c);
                } else if (iacCmd == WILL) {
                    sendDONT(sock, c);
                } else if (iacCmd == WONT || iacCmd == DONT) {
                    // 아무 작업도 하지 않음
                }
                parseState = STATE_DATA;
                break;
        }
    }
    return NULL;
}

// 송신 스레드 함수
void *sender_thread(void *arg) {
    char c;
    int n;

    while (1) {
        n = read(0, &c, 1);  // 한 문자씩 읽기
        if (n <= 0) {
            break;
        }
        if (write(sock, &c, 1) <= 0) {  // 한 문자씩 전송
            perror("write (client->server)");
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    struct termios raw_tios;
    pthread_t receiver_tid, sender_tid;
    
    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    
    // 터미널 설정
    tcgetattr(0, &org_tios);  // 현재 설정 저장
    raw_tios = org_tios;
    raw_tios.c_lflag &= ~(ECHO | ICANON | ISIG);  // 에코, 정규 모드, 시그널 비활성화
    raw_tios.c_cc[VMIN] = 1;   // 최소 1문자
    raw_tios.c_cc[VTIME] = 0;  // 타임아웃 없음
    tcsetattr(0, TCSANOW, &raw_tios);
    
    // SIGINT 핸들러 등록
    signal(SIGINT, sigint_handler);
    
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");
    
    // Nagle 알고리즘 비활성화
    int val = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&val, sizeof(val));
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");
    
    // 수신 스레드 생성
    if (pthread_create(&receiver_tid, NULL, receiver_thread, NULL) != 0) {
        error_handling("pthread_create() error");
    }
    
    // 송신 스레드 생성
    if (pthread_create(&sender_tid, NULL, sender_thread, NULL) != 0) {
        error_handling("pthread_create() error");
    }
    
    // 스레드 종료 대기
    pthread_join(receiver_tid, NULL);
    pthread_join(sender_tid, NULL);
    
    restore_terminal();
    close(sock);
    return 0;
} 