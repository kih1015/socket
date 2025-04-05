#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 31337
#define BUFFER_SIZE 1024

// 자식 프로세스 종료 처리 함수
void handle_child_signal(int signo) {
    pid_t pid;
    int status;
    
    // 모든 종료된 자식 프로세스를 정리
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("자식 프로세스 %d가 종료되었습니다.\n", pid);
    }
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("소켓 생성 실패");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("소켓 옵션 설정 실패");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("바인딩 실패");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("리스닝 실패");
        exit(EXIT_FAILURE);
    }

    // SIGCHLD 시그널 핸들러 설정
    struct sigaction sa;
    sa.sa_handler = handle_child_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("시그널 핸들러 설정 실패");
        exit(EXIT_FAILURE);
    }

    printf("멀티프로세싱 서버가 포트 %d에서 실행 중입니다...\n", PORT);

    while (1) {
        // Accept a new connection
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            perror("연결 수락 실패");
            continue; // Continue to accept next incoming connections
        }

        printf("클라이언트가 연결되었습니다: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Fork a new process to handle the client
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork() 실패");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            // Child process
            close(server_fd); // Child does not need the listening socket

            // Handle client communication
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t nbytes = read(client_fd, buffer, BUFFER_SIZE - 1);
                if (nbytes < 0) {
                    perror("읽기 실패");
                    break;
                } else if (nbytes == 0) {
                    // Connection closed by client
                    printf("클라이언트가 연결을 종료했습니다.\n");
                    break;
                }

                // Echo message back to the client
                printf("클라이언트로부터 받은 메시지: %s", buffer);
                if (write(client_fd, buffer, nbytes) < 0) {
                    perror("쓰기 실패");
                    break;
                }
            }
            close(client_fd);
            exit(EXIT_SUCCESS); // Terminate child process
        } else {
            // Parent process
            printf("새로운 자식 프로세스 생성: %d\n", pid);
            close(client_fd); // Parent does not need this connection socket
        }
    }

    close(server_fd);
    return 0;
} 