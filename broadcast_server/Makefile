# 컴파일러 및 컴파일 옵션 정의
CC = gcc
CFLAGS = -Wall

# 모든 소스 파일과 실행 파일 정의
TARGETS = broadcast_server broadcast_client

# 기본 타겟: 모든 실행 파일 빌드
all: $(TARGETS)

broadcast_server: broadcast_server.c
	$(CC) $(CFLAGS) -o broadcast_server broadcast_server.c

broadcast_client: broadcast_client.c
	$(CC) $(CFLAGS) -o broadcast_client broadcast_client.c

# 정리 규칙
clean:
	rm -f $(TARGETS)

# 가짜 타겟 선언
.PHONY: all clean 