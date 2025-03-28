# Unix 도메인 소켓 통신 프로젝트 보고서

## 1. 프로젝트 개요

본 프로젝트는 Unix 도메인 소켓(Unix Domain Socket)을 이용한 클라이언트-서버 통신 구현에 관한 것입니다. Unix 도메인 소켓은 같은 호스트 내에서 프로세스 간 통신(IPC, Inter-Process Communication)을 위한 메커니즘으로, 네트워크 소켓과 유사한 API를 제공하지만 네트워크 스택을 거치지 않아 더 효율적인 통신이 가능합니다.

## 2. 구현 환경

- 운영체제: Linux/Unix 계열 (우분투 등)
- 프로그래밍 언어: C
- 컴파일러: GCC
- 빌드 도구: Make

## 3. 프로젝트 구조

- `client.c`: Unix 도메인 소켓 클라이언트 구현
- `server.c`: Unix 도메인 소켓 서버 구현
- `Makefile`: 빌드 스크립트
- 실행 파일:
  - `unix_client`: 클라이언트 프로그램
  - `unix_server`: 서버 프로그램

## 4. 소켓 통신 구현

### 4.1. 서버 구현 (server.c)

서버는 다음과 같은 단계로 구현되었습니다:

1. 소켓 생성 (`socket()`)
2. 소켓 주소 구조체 초기화
3. 소켓을 파일 시스템의 경로에 바인딩 (`bind()`)
4. 연결 요청 대기 모드로 전환 (`listen()`)
5. 클라이언트 연결 수락 (`accept()`)
6. 클라이언트로부터 데이터 수신 (`recv()`)
7. 수신된 메시지 출력
8. 소켓 종료 (`close()`)

### 4.2. 클라이언트 구현 (client.c)

클라이언트는 다음과 같은 단계로 구현되었습니다:

1. 소켓 생성 (`socket()`)
2. 소켓 주소 구조체 초기화
3. 서버 소켓에 연결 시도 (`connect()`)
4. 메시지 전송 (`send()`)
5. 소켓 종료 (`close()`)

## 5. 코드 설명

### 5.1. 서버 코드 (server.c)

```c
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SOCKET_NAME "hbsocket"

int main() {
    char buf[256];
    struct sockaddr_un ser, cli;
    int sd, nsd, len, clen;

    // 소켓 생성
    if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 서버 주소 구조체 초기화
    memset((char *)&ser, 0, sizeof(struct sockaddr_un));
    ser.sun_family = AF_UNIX;
    strcpy(ser.sun_path, SOCKET_NAME);
    len = sizeof(ser.sun_family) + strlen(ser.sun_path);

    // 소켓 바인딩
    if (bind(sd, (struct sockaddr *)&ser, len)) {
        perror("bind");
        exit(1);
    }

    // 연결 대기
    if (listen(sd, 5) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Waiting ...\n");
    
    // 클라이언트 연결 수락
    if ((nsd = accept(sd, (struct sockaddr *)&cli, &clen)) == -1) {
        perror("accept");
        exit(1);
    }

    // 데이터 수신
    if (recv(nsd, buf, sizeof(buf), 0) == -1) {
        perror("recv");
        exit(1);
    }

    printf("Received Message: %s\n", buf);
    
    // 소켓 종료
    close(nsd);
    close(sd);
}
```

### 5.2. 클라이언트 코드 (client.c)

```c
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SOCKET_NAME "hbsocket"

int main() {
    int sd, len;
    char buf[256];
    struct sockaddr_un ser;
    
    // 소켓 생성
    if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    // 서버 주소 구조체 초기화
    memset((char *)&ser, '\0', sizeof(ser));
    ser.sun_family = AF_UNIX;
    strcpy(ser.sun_path, SOCKET_NAME);
    len = sizeof(ser.sun_family) + strlen(ser.sun_path);
    
    // 서버에 연결
    if (connect(sd, (struct sockaddr *)&ser, len) < 0) {
        perror("connect");
        exit(1);
    }
    
    // 메시지 전송
    strcpy(buf, "Unix Domain Socket Test Message");
    if (send(sd, buf, sizeof(buf), 0) == -1) {
        perror("send");
        exit(1);
    }
    
    // 소켓 종료
    close(sd);
}
```

## 6. 빌드 및 실행 방법

### 6.1. 빌드

프로젝트를 빌드하기 위해 다음 명령어를 실행합니다:

```bash
make
```

이 명령은 `client.c`와 `server.c`를 컴파일하여 각각 `unix_client`와 `unix_server` 실행 파일을 생성합니다.

### 6.2. 실행

1. 먼저 서버를 실행합니다:

```bash
./unix_server
```

서버는 "Waiting ..." 메시지를 출력하고 클라이언트의 연결을 기다립니다.

2. 다른 터미널에서 클라이언트를 실행합니다:

```bash
./unix_client
```

클라이언트는 서버에 연결하여 "Unix Domain Socket Test Message" 메시지를 전송합니다.

3. 서버는 클라이언트로부터 메시지를 수신하고 출력한 후 종료됩니다.

## 7. Unix 도메인 소켓의 특징

1. **로컬 통신**: 같은 호스트 내 프로세스 간 통신에 사용됩니다.
2. **파일 시스템 기반**: 소켓은 파일 시스템에 경로로 표현됩니다.
3. **효율성**: 네트워크 스택을 거치지 않아 TCP/IP 소켓보다 더 효율적입니다.
4. **보안**: 로컬 시스템 내에서만 통신하므로 네트워크 보안 위험이 없습니다.
5. **신뢰성**: 데이터 손실이나 순서 변경 없이 안정적인 데이터 전송이 가능합니다.

## 8. 결론

본 프로젝트에서는 Unix 도메인 소켓을 이용한 클라이언트-서버 통신 모델을 구현했습니다. 이 구현은 프로세스 간 통신의 기본적인 방법을 보여주며, 같은 호스트 내에서 효율적인 IPC 메커니즘으로 활용될 수 있습니다. 서버 측에서는 소켓 생성, 바인딩, 리스닝 및 데이터 수신 과정을, 클라이언트 측에서는 소켓 생성, 연결 및 데이터 전송 과정을 구현하여 Unix 도메인 소켓 통신의 기본 원리를 학습할 수 있었습니다.

## 9. 참고 자료

- Stevens, W. R. (2003). UNIX Network Programming, Volume 1: The Sockets Networking API (3rd ed.). Addison-Wesley.
- Kerrisk, M. (2010). The Linux Programming Interface. No Starch Press.
- 리눅스 매뉴얼 페이지: socket(2), bind(2), listen(2), accept(2), connect(2), send(2), recv(2), close(2) 