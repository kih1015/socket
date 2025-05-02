# 컴파일러 및 컴파일 옵션 정의
CC = gcc
CFLAGS = -Wall -Wextra

# 모든 소스 파일과 실행 파일 정의
TARGET = broadcast_server
POLL_TARGET = broadcast_server_poll
EPOLL_TARGET = broadcast_server_epoll
CLIENT_TARGET = broadcast_client

# 기본 타겟: 모든 실행 파일 빌드
all: $(TARGET) $(POLL_TARGET) $(EPOLL_TARGET) $(CLIENT_TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

$(POLL_TARGET): $(POLL_TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

$(EPOLL_TARGET): $(EPOLL_TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

$(CLIENT_TARGET): $(CLIENT_TARGET).c
	$(CC) $(CFLAGS) -o $@ $<

# 정리 규칙
clean:
	rm -f $(TARGET) $(POLL_TARGET) $(EPOLL_TARGET) $(CLIENT_TARGET)

# 가짜 타겟 선언
.PHONY: all clean 