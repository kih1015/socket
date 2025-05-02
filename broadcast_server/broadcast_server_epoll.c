#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <errno.h>
#include <fcntl.h>

#define SERVERPORT 9000
#define BUFSIZE 512
#define MAX_EVENTS 1024

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
void broadcast_message(int epoll_fd, int server_sock, char *msg, int msg_len) {
    struct epoll_event events[MAX_EVENTS];
    int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 0);
    
    for (int i = 0; i < nfds; i++) {
        if (events[i].data.fd != server_sock) {
            int retval = send(events[i].data.fd, msg, msg_len, 0);
            if (retval == -1 && errno != EWOULDBLOCK) {
                printf("브로드캐스트 실패 - socket: %d\n", events[i].data.fd);
            }
        }
    }
}

int main() {
    int server_sock, epoll_fd;
    struct sockaddr_in server_addr;
    char buf[BUFSIZE];
    int retval;
    
    // epoll 인스턴스 생성
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) err_quit("epoll_create1()");
    
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
    
    // 서버 소켓을 epoll에 등록
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &ev) == -1) {
        err_quit("epoll_ctl()");
    }
    
    while (1) {
        struct epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 10);  // 10ms 타임아웃
        
        if (nfds == -1) {
            if (errno == EINTR) continue;
            err_quit("epoll_wait()");
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_sock) {
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
                
                // 새 소켓을 epoll에 등록
                ev.events = EPOLLIN | EPOLLET;  // Edge-triggered 모드
                ev.data.fd = client_sock;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &ev) == -1) {
                    err_quit("epoll_ctl()");
                }
                
                printf("\n[TCP 서버] 클라이언트 접속: socket=%d\n", client_sock);
            }
            else {
                // 클라이언트로부터 데이터 수신
                memset(buf, 0, BUFSIZE);
                retval = recv(events[i].data.fd, buf, BUFSIZE, 0);
                
                if (retval == -1) {
                    if (errno != EWOULDBLOCK) {
                        printf("recv() 오류\n");
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                        close(events[i].data.fd);
                        printf("[TCP 서버] 클라이언트 종료: socket=%d\n", events[i].data.fd);
                    }
                    continue;
                }
                else if (retval == 0) {
                    // 클라이언트 정상 종료
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                    close(events[i].data.fd);
                    printf("[TCP 서버] 클라이언트 종료: socket=%d\n", events[i].data.fd);
                    continue;
                }
                
                // 받은 데이터 출력
                printf("[TCP/socket=%d] %s\n", events[i].data.fd, buf);
                
                // 모든 클라이언트에게 브로드캐스트
                broadcast_message(epoll_fd, server_sock, buf, retval);
            }
        }
    }
    
    close(epoll_fd);
    close(server_sock);
    return 0;
}
