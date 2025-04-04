#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 31337
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char send_buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE];
    int recv_len;
    char student_id[20];
    long long received_number, result;
    
    // 소켓 생성
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("소켓 생성 실패");
        exit(1);
    }
    
    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    
    // 서버 IP 주소 변환
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("주소 변환 실패");
        exit(1);
    }
    
    // 서버에 연결
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("연결 실패");
        exit(1);
    }
    
    printf("서버에 연결되었습니다\n", SERVER_IP, SERVER_PORT);
    
    // 학번 입력 받기
    printf("학번을 입력하세요: ");
    scanf("%s", student_id);
    getchar(); // 버퍼 비우기
    
    // 학번을 서버에 전송
    if (send(sock, student_id, strlen(student_id), 0) < 0) {
        perror("학번 전송 실패");
        close(sock);
        exit(1);
    }
    
    printf("학번 전송: %s\n", student_id);
    
    // 서버로부터 응답 수신
    memset(recv_buffer, 0, BUFFER_SIZE);
    if ((recv_len = recv(sock, recv_buffer, BUFFER_SIZE, 0)) < 0) {
        perror("수신 실패");
        close(sock);
        exit(1);
    }
    
    recv_buffer[recv_len] = '\0';
    printf("서버 응답: %s\n", recv_buffer);
    
    // 받은 숫자를 long long 타입으로 변환
    received_number = atoll(recv_buffer);
    
    // 학번을 long long 타입으로 변환 후 더하기
    result = received_number + atoll(student_id);
    
    // 결과를 문자열로 변환하여 다시 보내기
    sprintf(send_buffer, "%lld", result);
    
    printf("서버 응답 + 학번: %s\n", send_buffer);
    
    // 계산 결과 전송
    if (send(sock, send_buffer, strlen(send_buffer), 0) < 0) {
        perror("결과 전송 실패");
        close(sock);
        exit(1);
    }
    
    // 서버로부터 최종 응답 수신
    memset(recv_buffer, 0, BUFFER_SIZE);
    if ((recv_len = recv(sock, recv_buffer, BUFFER_SIZE, 0)) < 0) {
        perror("최종 응답 수신 실패");
        close(sock);
        exit(1);
    }
    
    recv_buffer[recv_len] = '\0';
    printf("서버 최종 응답: %s\n", recv_buffer);
    
    // 소켓 종료
    close(sock);
    
    return 0;
} 