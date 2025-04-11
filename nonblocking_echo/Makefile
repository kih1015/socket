# 컴파일러 및 컴파일 옵션 정의
CC = gcc
CFLAGS = -Wall

# 모든 소스 파일과 실행 파일 정의
TARGETS = nb_echo_server nb_echo_client

# 기본 타겟: 모든 실행 파일 빌드
all: $(TARGETS)

nb_echo_server: nb_echo_server.c
	$(CC) $(CFLAGS) -o nb_echo_server nb_echo_server.c

nb_echo_client: nb_echo_client.c
	$(CC) $(CFLAGS) -o nb_echo_client nb_echo_client.c

# 정리 규칙
clean:
	rm -f $(TARGETS)

# 가짜 타겟 선언
.PHONY: all clean 