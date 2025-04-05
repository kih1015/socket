#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void receive_data(int sock_fd) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int recv_len = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr *)&client_addr, &client_len);
        
        if (recv_len > 0) {
            printf("수신: %s\n", buffer);
        }
    }
}

void send_data(int sock_fd, const char *remote_ip, int remote_port) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(remote_port);
    server_addr.sin_addr.s_addr = inet_addr(remote_ip);

    while (1) {
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // 개행 문자 제거

        if (strlen(buffer) > 0) {
            sendto(sock_fd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("사용법: %s <local_port> <remote_ip> <remote_port>\n", argv[0]);
        return 1;
    }

    int local_port = atoi(argv[1]);
    const char *remote_ip = argv[2];
    int remote_port = atoi(argv[3]);

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("소켓 생성 실패");
        return 1;
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(local_port);

    if (bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("바인딩 실패");
        close(sock_fd);
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork 실패");
        close(sock_fd);
        return 1;
    }

    if (pid == 0) {
        // 자식 프로세스: 데이터 수신
        receive_data(sock_fd);
    } else {
        // 부모 프로세스: 데이터 송신
        send_data(sock_fd, remote_ip, remote_port);
    }

    close(sock_fd);
    return 0;
} 
