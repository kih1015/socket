#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 31337
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    char final_response[BUFFER_SIZE];
    int recv_len;
    long long random_number, client_number, expected_result, received_result;
    
    // 소켓 생성
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("소켓 생성 실패");
        exit(1);
    }
    
    // 소켓 옵션 설정 (재사용 가능하도록)
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("소켓 옵션 설정 실패");
        exit(1);
    }
    
    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 소켓 바인딩
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("바인딩 실패");
        exit(1);
    }
    
    // 연결 대기
    if (listen(server_fd, 5) < 0) {
        perror("리스닝 실패");
        exit(1);
    }
    
    printf("서버가 포트 %d에서 실행 중입니다...\n", PORT);
    
    // 클라이언트 연결 수락
    if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
        perror("연결 수락 실패");
        exit(1);
    }
    
    printf("클라이언트가 연결되었습니다: %s\n", inet_ntoa(client_addr.sin_addr));
    
    // 클라이언트로부터 학번 수신
    memset(buffer, 0, BUFFER_SIZE);
    if ((recv_len = recv(client_fd, buffer, BUFFER_SIZE, 0)) < 0) {
        perror("수신 실패");
        close(client_fd);
        close(server_fd);
        exit(1);
    }
    
    buffer[recv_len] = '\0';
    printf("클라이언트로부터 받은 학번: %s\n", buffer);
    
    // 학번을 숫자로 변환
    client_number = atoll(buffer);
    
    // 랜덤 숫자 생성 (18자리)
    srand(time(NULL));
    random_number = rand() % 1000000000000000000LL;
    sprintf(response, "%lld", random_number);
    
    // 랜덤 숫자를 클라이언트에게 전송
    if (send(client_fd, response, strlen(response), 0) < 0) {
        perror("랜덤 숫자 전송 실패");
        close(client_fd);
        close(server_fd);
        exit(1);
    }
    
    printf("클라이언트에게 전송한 랜덤 숫자: %s\n", response);
    
    // 클라이언트로부터 계산 결과 수신
    memset(buffer, 0, BUFFER_SIZE);
    if ((recv_len = recv(client_fd, buffer, BUFFER_SIZE, 0)) < 0) {
        perror("계산 결과 수신 실패");
        close(client_fd);
        close(server_fd);
        exit(1);
    }
    
    buffer[recv_len] = '\0';
    printf("클라이언트로부터 받은 계산 결과: %s\n", buffer);
    
    // 클라이언트가 보낸 계산 결과를 숫자로 변환
    received_result = atoll(buffer);
    
    // 서버에서 예상하는 계산 결과 (학번 + 랜덤 숫자)
    expected_result = client_number + random_number;
    
    // 계산 결과가 일치하는지 확인
    if (received_result == expected_result) {
        strcpy(final_response, "success");
        printf("계산 결과가 일치합니다. 응답: %s\n", final_response);
    } else {
        strcpy(final_response, "fail");
        printf("계산 결과가 일치하지 않습니다. 응답: %s\n", final_response);
        printf("예상 결과: %lld, 받은 결과: %lld\n", expected_result, received_result);
    }
    
    // 최종 응답을 클라이언트에게 전송
    if (send(client_fd, final_response, strlen(final_response), 0) < 0) {
        perror("최종 응답 전송 실패");
        close(client_fd);
        close(server_fd);
        exit(1);
    }
    
    printf("클라이언트에게 전송한 최종 응답: %s\n", final_response);
    
    // 소켓 종료
    close(client_fd);
    close(server_fd);
    
    return 0;
} 