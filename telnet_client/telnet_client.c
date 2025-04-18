#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

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

// 전역 변수로 자식 프로세스 ID 저장
pid_t childPid = -1;

void error_handling(char *message) {
    perror(message);
    exit(1);
}

// SIGINT 핸들러
void sigint_handler(int signo) {
    if (childPid > 0) {
        kill(childPid, SIGTERM);
    }
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

// 수신 프로세스
void receiver_process(int sock, pid_t parentPid) {
    int parseState = STATE_DATA;
    unsigned char c;
    unsigned char iacCmd;
    int n;

    while (1) {
        n = read(sock, &c, 1);
        
        if (n < 0) {
            perror("read (server->client)");
            kill(parentPid, SIGTERM);
            exit(EXIT_FAILURE);
        } else if (n == 0) {
            fprintf(stderr, "\n[INFO] Server closed connection.\n");
            kill(parentPid, SIGTERM);
            exit(EXIT_SUCCESS);
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
}

// 송신 프로세스
void sender_process(int sock) {
    char buf[1024];
    int str_len;

    while (1) {
        str_len = read(0, buf, sizeof(buf));
        if (str_len <= 0) {
            break;
        }
        if (write(sock, buf, str_len) <= 0) {
            perror("write (client->server)");
            break;
        }
    }

    // 송신 프로세스가 종료되면 수신 프로세스도 종료
    if (childPid > 0) {
        kill(childPid, SIGTERM);
    }
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    
    if (argc != 3) {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    
    // SIGINT 핸들러 등록
    signal(SIGINT, sigint_handler);
    
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");
    
    childPid = fork();
    if (childPid < 0) {
        error_handling("fork() error");
    } else if (childPid == 0) {  // 자식 프로세스: 수신부
        receiver_process(sock, getppid());
    } else {  // 부모 프로세스: 송신부
        sender_process(sock);
    }
    
    close(sock);
    return 0;
}
