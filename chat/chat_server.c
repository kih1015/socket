/*
 * 간단한 C 채팅 서버
 * 요구사항:
 * - 여러 클라이언트 동시 접속 지원
 * - 접속 시 닉네임 입력 요청
 * - 로그아웃 또는 연결 종료 시까지 클라이언트 목록 관리
 * - 연결된 모든 클라이언트에 메시지 브로드캐스트
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 12345
#define MAX_CLIENTS FD_SETSIZE
#define BUF_SIZE 1024
#define NAME_LEN 32

int main() {
    int listener, newfd;
    struct sockaddr_in server_addr, client_addr;
    fd_set master_set, read_fds;
    int fdmax, i;
    int client_fds[MAX_CLIENTS];
    char client_names[MAX_CLIENTS][NAME_LEN];
    socklen_t addrlen;
    char buf[BUF_SIZE];          // 클라이언트로부터 받은 메시지 저장 버퍼
    char msg[BUF_SIZE];          // 브로드캐스트용 메시지 포맷 버퍼
    int nbytes;

    // client_fds를 -1로 초기화
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
        client_names[i][0] = '\0';
    }

    // 리스너 소켓 생성
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // 주소 재사용 허용
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 소켓에 주소 바인딩
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // 연결 요청 대기 시작
    if (listen(listener, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // fd_set 초기화
    FD_ZERO(&master_set);
    FD_ZERO(&read_fds);
    FD_SET(listener, &master_set);
    fdmax = listener;

    printf("포트 %d에서 채팅 서버 시작\n", PORT);

    // 메인 루프
    while (1) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        // 모든 파일 디스크립터 확인
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    // 새 연결 처리
                    addrlen = sizeof(client_addr);
                    newfd = accept(listener, (struct sockaddr*)&client_addr, &addrlen);
                    if (newfd < 0) {
                        perror("accept");
                        continue;
                    }
                    // client_fds 배열에 추가
                    int j;
                    for (j = 0; j < MAX_CLIENTS; j++) {
                        if (client_fds[j] < 0) {
                            client_fds[j] = newfd;
                            break;
                        }
                    }
                    if (j == MAX_CLIENTS) {
                        fprintf(stderr, "최대 클라이언트 수 초과. 연결 거부됨.\n");
                        close(newfd);
                    } else {
                        FD_SET(newfd, &master_set);
                        if (newfd > fdmax) fdmax = newfd;
                        // 닉네임 입력 요청
                        send(newfd, "닉네임을 입력하세요: ", 21, 0);
                        // 닉네임 수신
                        if ((nbytes = recv(newfd, buf, NAME_LEN-1, 0)) <= 0) {
                            close(newfd);
                            FD_CLR(newfd, &master_set);
                            client_fds[j] = -1;
                        } else {
                            buf[nbytes] = '\0';
                            // 개행 문자 제거
                            buf[strcspn(buf, "\r\n")] = '\0';
                            strncpy(client_names[j], buf, NAME_LEN);
                            // 입장 메시지 생성
                            snprintf(buf, sizeof(buf), "%s님이 입장하였습니다.\n", client_names[j]);
                            // 입장 메시지 브로드캐스트
                            for (int k = 0; k < MAX_CLIENTS; k++) {
                                if (client_fds[k] >= 0 && k != j) {
                                    send(client_fds[k], buf, strlen(buf), 0);
                                }
                            }
                            printf("%s 연결됨 (%s:%d)\n", client_names[j], 
                                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                        }
                    }
                } else {
                    // 클라이언트로부터 데이터 수신
                    int idx;
                    // 인덱스 찾기
                    for (idx = 0; idx < MAX_CLIENTS; idx++) {
                        if (client_fds[idx] == i) break;
                    }
                    if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
                        // 연결 종료 처리
                        close(i);
                        FD_CLR(i, &master_set);
                        if (idx < MAX_CLIENTS) {
                            snprintf(buf, sizeof(buf), "%s님이 퇴장하였습니다.\n", client_names[idx]);
                            // 퇴장 메시지 브로드캐스트
                            for (int k = 0; k < MAX_CLIENTS; k++) {
                                if (client_fds[k] >= 0) send(client_fds[k], buf, strlen(buf), 0);
                            }
                            client_fds[idx] = -1;
                            client_names[idx][0] = '\0';
                        }
                    } else {
                        buf[nbytes] = '\0';
                        // 개행 문자 제거
                        buf[strcspn(buf, "\r\n")] = '\0';
                        // /quit 명령 처리
                        if (strcmp(buf, "/quit") == 0) {
                            close(i);
                            FD_CLR(i, &master_set);
                            if (idx < MAX_CLIENTS) {
                                snprintf(buf, sizeof(buf), "%s님이 퇴장하였습니다.\n", client_names[idx]);
                                for (int k = 0; k < MAX_CLIENTS; k++) {
                                    if (client_fds[k] >= 0) send(client_fds[k], buf, strlen(buf), 0);
                                }
                                client_fds[idx] = -1;
                                client_names[idx][0] = '\0';
                            }
                        } else {
                            // 클라이언트 메시지 브로드캐스트
                            snprintf(msg, sizeof(msg), "%s: %s\n", client_names[idx], buf);
                            for (int k = 0; k < MAX_CLIENTS; k++) {
                                if (client_fds[k] >= 0 && client_fds[k] != i) {
                                    send(client_fds[k], msg, strlen(msg), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
