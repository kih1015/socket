# 컴파일러 및 컴파일 옵션 정의
CC = gcc
CFLAGS = -Wall -Wextra

# 모든 소스 파일과 실행 파일 정의
TARGETS = unix_client unix_server inet_client inet_server student_id_client student_id_server multi_process_client multi_process_server udp_peer udp_client udp_server

# 기본 타겟: 모든 실행 파일 빌드
all: unix_socket inet_socket student_id multi_process udp_peer udp

# 각 소스 파일에 대한 빌드 규칙
unix_socket:
	$(MAKE) -C $@

inet_socket:
	$(MAKE) -C $@

student_id:
	$(MAKE) -C $@

multi_process:
	$(MAKE) -C $@

udp_peer:
	$(MAKE) -C $@

udp:
	$(MAKE) -C $@

# 정리 규칙
clean:
	$(MAKE) -C unix_socket clean
	$(MAKE) -C inet_socket clean
	$(MAKE) -C student_id clean
	$(MAKE) -C multi_process clean
	$(MAKE) -C udp_peer clean
	$(MAKE) -C udp clean

# 가짜 타겟 선언
.PHONY: all clean unix_socket inet_socket student_id multi_process udp_peer udp
