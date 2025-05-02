#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

#define SERVERPORT 9000
#define BUFSIZE 512
#define MAX_CLIENTS 1024

void err_quit(const char *msg) {
    perror(msg);
    exit(1);
}

// 소켓을 넌블로킹 모드로 설정
void set_nonblocking_mode(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

// 모든 클라이언트에게 메시지 브로드캐스트
void broadcast_message(struct pollfd *fds, int nfds, int server_sock, char *msg, int msg_len) {
    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd != server_sock && fds[i].fd != -1) {
            int retval = send(fds[i].fd, msg, msg_len, 0);
            if (retval == -1 && errno != EWOULDBLOCK) {
                printf("브로드캐스트 실패 - socket: %d\n", fds[i].fd);
            }
        }
    }
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;
    char buf[BUFSIZE];
    int retval;
    
    // pollfd 구조체 배열 초기화
    struct pollfd fds[MAX_CLIENTS];
    int nfds = 1;  // 서버 소켓만 있음
    
    // 모든 pollfd 구조체 초기화
    for (int i = 0; i < MAX_CLIENTS; i++) {
        fds[i].fd = -1;
        fds[i].events = POLLIN;
        fds[i].revents = 0;
    }
    
    // 소켓 생성
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) err_quit("socket()");
    
    // 서버 소켓을 넌블로킹 모드로 설정
    set_nonblocking_mode(server_sock);
    
    // SO_REUSEADDR 소켓 옵션 설정
    int optval = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (retval == -1) err_quit("setsockopt()");
    
    // bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVERPORT);
    retval = bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (retval == -1) err_quit("bind()");
    
    // listen
    retval = listen(server_sock, SOMAXCONN);
    if (retval == -1) err_quit("listen()");
    
    printf("브로드캐스트 서버 시작...\n");
    
    // 서버 소켓을 pollfd 배열에 추가
    fds[0].fd = server_sock;
    fds[0].events = POLLIN;
    
    while (1) {
        usleep(10000);  // CPU 사용량 제어
        
        // poll() 호출
        retval = poll(fds, nfds, 10);  // 10ms 타임아웃
        
        if (retval == -1) {
            if (errno == EINTR) continue;
            err_quit("poll()");
            break;
        }
        else if (retval == 0) {
            continue;  // 타임아웃
        }
        
        // 모든 소켓 확인
        for (int i = 0; i < nfds; i++) {
            if (fds[i].revents == 0) continue;
            
            if (fds[i].fd == server_sock) {
                // 새 클라이언트 연결 처리
                struct sockaddr_in client_addr;
                socklen_t client_addr_size = sizeof(client_addr);
                int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
                
                if (client_sock == -1) {
                    if (errno != EWOULDBLOCK) printf("accept() 오류\n");
                    continue;
                }
                
                // 클라이언트 소켓을 넌블로킹 모드로 설정
                set_nonblocking_mode(client_sock);
                
                // 새 소켓을 pollfd 배열에 추가
                if (nfds < MAX_CLIENTS) {
                    fds[nfds].fd = client_sock;
                    fds[nfds].events = POLLIN;
                    nfds++;
                    
                    printf("\n[TCP 서버] 클라이언트 접속: socket=%d\n", client_sock);
                } else {
                    printf("최대 클라이언트 수 초과\n");
                    close(client_sock);
                }
            }
            else {
                // 클라이언트로부터 데이터 수신
                memset(buf, 0, BUFSIZE);
                retval = recv(fds[i].fd, buf, BUFSIZE, 0);
                
                if (retval == -1) {
                    if (errno != EWOULDBLOCK) {
                        printf("recv() 오류\n");
                        close(fds[i].fd);
                        fds[i].fd = -1;
                        printf("[TCP 서버] 클라이언트 종료: socket=%d\n", fds[i].fd);
                    }
                    continue;
                }
                else if (retval == 0) {
                    // 클라이언트 정상 종료
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    printf("[TCP 서버] 클라이언트 종료: socket=%d\n", fds[i].fd);
                    continue;
                }
                
                // 받은 데이터 출력
                printf("[TCP/socket=%d] %s\n", fds[i].fd, buf);
                
                // 모든 클라이언트에게 브로드캐스트
                broadcast_message(fds, nfds, server_sock, buf, retval);
            }
        }
    }
    
    // 모든 소켓 닫기
    for (int i = 0; i < nfds; i++) {
        if (fds[i].fd != -1) {
            close(fds[i].fd);
        }
    }
    
    return 0;
} 