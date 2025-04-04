CC = gcc
CFLAGS = -Wall -Wextra -g

# 모든 소스 파일과 실행 파일 정의
CLIENT = client
SERVER = server
INET_CLIENT = inet_client
INET_SERVER = inet_server
STUDENT_ID_CLIENT = student_id_client
STUDENT_ID_SERVER = student_id_server
MULTI_PROCESS_CLIENT = multi_process_client
MULTI_PROCESS_SERVER = multi_process_server

# 모든 타겟 정의
TARGETS = $(CLIENT) $(SERVER) $(INET_CLIENT) $(INET_SERVER) \
          $(STUDENT_ID_CLIENT) $(STUDENT_ID_SERVER) \
          $(MULTI_PROCESS_CLIENT) $(MULTI_PROCESS_SERVER)

# 기본 타겟: 모든 실행 파일 빌드
all: $(TARGETS)

# 각 소스 파일에 대한 빌드 규칙
$(CLIENT): client.c
	$(CC) $(CFLAGS) -o $@ $<

$(SERVER): server.c
	$(CC) $(CFLAGS) -o $@ $<

$(INET_CLIENT): inet_client.c
	$(CC) $(CFLAGS) -o $@ $<

$(INET_SERVER): inet_server.c
	$(CC) $(CFLAGS) -o $@ $<

$(STUDENT_ID_CLIENT): student_id_client.c
	$(CC) $(CFLAGS) -o $@ $<

$(STUDENT_ID_SERVER): student_id_server.c
	$(CC) $(CFLAGS) -o $@ $<

$(MULTI_PROCESS_CLIENT): multi_process_client.c
	$(CC) $(CFLAGS) -o $@ $<

$(MULTI_PROCESS_SERVER): multi_process_server.c
	$(CC) $(CFLAGS) -o $@ $<

# 정리 규칙
clean:
	rm -f $(TARGETS)

# 가짜 타겟 선언
.PHONY: all clean 
