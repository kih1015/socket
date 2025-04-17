#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>  // fcntl() 함수를 위한 헤더

#define SERVERPORT 9000
#define BUFSIZE 512

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
void broadcast_message(fd_set *active_socks, int server_sock, char *msg, int msg_len, int sender_sock) {
    for (int i = 0; i <= FD_SETSIZE; i++) {
        if (i != server_sock && FD_ISSET(i, active_socks) && i != sender_sock) {
            int retval = send(i, msg, msg_len, 0);
            if (retval == -1 && errno != EWOULDBLOCK) {
                printf("브로드캐스트 실패 - socket: %d\n", i);
            }
        }
    }
}

int main() {
    int server_sock;
    struct sockaddr_in server_addr;
    char buf[BUFSIZE];
    int retval;
    
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
    
    // 활성 소켓 관리를 위한 fd_set
    fd_set active_socks, read_socks;
    FD_ZERO(&active_socks);
    FD_SET(server_sock, &active_socks);
    int maxfd = server_sock;
    
    while (1) {
        usleep(10000);  // CPU 사용량 제어
        
        read_socks = active_socks;  // select() 호출을 위해 복사
        
        struct timeval tv = {0, 10000};  // 10ms 타임아웃
        retval = select(maxfd + 1, &read_socks, NULL, NULL, &tv);
        
        if (retval == -1) {
            if (errno == EINTR) continue;
            err_quit("select()");
            break;
        }
        else if (retval == 0) {
            continue;  // 타임아웃
        }
        
        // 모든 소켓 확인
        for (int sock = 0; sock <= maxfd; sock++) {
            if (!FD_ISSET(sock, &read_socks)) continue;
            
            if (sock == server_sock) {
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
                
                // 새 소켓을 fd_set에 추가
                FD_SET(client_sock, &active_socks);
                if (client_sock > maxfd) maxfd = client_sock;
                
                printf("\n[TCP 서버] 클라이언트 접속: socket=%d\n", client_sock);
            }
            else {
                // 클라이언트로부터 데이터 수신
                memset(buf, 0, BUFSIZE);
                retval = recv(sock, buf, BUFSIZE, 0);
                
                if (retval == -1) {
                    if (errno != EWOULDBLOCK) {
                        printf("recv() 오류\n");
                        FD_CLR(sock, &active_socks);
                        close(sock);
                        printf("[TCP 서버] 클라이언트 종료: socket=%d\n", sock);
                    }
                    continue;
                }
                else if (retval == 0) {
                    // 클라이언트 정상 종료
                    FD_CLR(sock, &active_socks);
                    close(sock);
                    printf("[TCP 서버] 클라이언트 종료: socket=%d\n", sock);
                    continue;
                }
                
                // 받은 데이터 출력
                printf("[TCP/socket=%d] %s\n", sock, buf);
                
                // 모든 클라이언트에게 브로드캐스트
                broadcast_message(&active_socks, server_sock, buf, retval, sock);
            }
        }
    }
    
    // 모든 소켓 닫기
    for (int sock = 0; sock <= maxfd; sock++) {
        if (FD_ISSET(sock, &active_socks)) {
            close(sock);
        }
    }
    
    return 0;
} 