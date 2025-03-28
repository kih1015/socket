# Socket 통신 프로젝트

## 1. 프로젝트 개요

본 프로젝트는 소켓 통신을 이용한 클라이언트-서버 통신 구현에 관한 것입니다. 두 가지 타입의 소켓 통신을 구현했습니다:

1. **Unix 도메인 소켓 통신**: 같은 호스트 내에서 프로세스 간 통신(IPC)을 위한 메커니즘으로, 네트워크 스택을 거치지 않아 더 효율적인 통신이 가능합니다.
2. **인터넷 소켓 통신(에코 서버)**: TCP/IP 프로토콜을 사용하여 클라이언트와 서버가 네트워크를 통해 통신하며, 클라이언트가 보낸 메시지를 서버가 그대로 반환하는 에코 서비스를 제공합니다.

## 2. 구현 환경

- 운영체제: Linux/Unix 계열 (우분투 등)
- 프로그래밍 언어: C
- 컴파일러: GCC
- 빌드 도구: Make

## 3. 프로젝트 구조

- `client.c`: Unix 도메인 소켓 클라이언트 구현
- `server.c`: Unix 도메인 소켓 서버 구현
- `inet_client.c`: 인터넷 소켓 에코 클라이언트 구현
- `inet_server.c`: 인터넷 소켓 에코 서버 구현
- `Makefile`: 빌드 스크립트
- 실행 파일:
  - `unix_client`: Unix 도메인 소켓 클라이언트
  - `unix_server`: Unix 도메인 소켓 서버
  - `inet_client`: 인터넷 소켓 에코 클라이언트
  - `inet_server`: 인터넷 소켓 에코 서버

## 4. Unix 도메인 소켓 통신 구현

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

## 5. 인터넷 소켓 에코 서버 구현

### 5.1. 에코 서버 구현 (inet_server.c)

에코 서버는 다음과 같은 단계로 구현되었습니다:

1. 소켓 생성 (`socket()`)
2. 소켓 주소 구조체 초기화 - INADDR_ANY 사용하여 모든 인터페이스에서 연결 수락
3. 지정된 포트(9000)에 소켓 바인딩 (`bind()`)
4. 연결 요청 대기 모드로 전환 (`listen()`)
5. 클라이언트 연결 수락 (`accept()`)
6. 무한 루프를 돌며 다음을 수행:
   - 클라이언트로부터 데이터 수신 (`recv()`)
   - 수신된 메시지를 그대로 클라이언트에게 다시 전송 (`send()`)
7. 연결 종료 시 소켓 종료 (`close()`)

### 5.2. 에코 클라이언트 구현 (inet_client.c)

에코 클라이언트는 다음과 같은 단계로 구현되었습니다:

1. 소켓 생성 (`socket()`)
2. 소켓 주소 구조체 초기화 - 서버 IP 주소와 포트 설정
3. 서버 소켓에 연결 시도 (`connect()`)
4. 무한 루프를 돌며 다음을 수행:
   - 사용자로부터 메시지 입력 받기
   - 'exit' 입력 시 종료
   - 입력 메시지를 서버에 전송 (`send()`)
   - 서버로부터 에코된 메시지 수신 (`recv()`)
   - 수신된 메시지 출력
5. 종료 시 소켓 종료 (`close()`)

## 6. 코드 설명

### 6.1. Unix 도메인 소켓 서버 코드 (server.c)

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

### 6.2. Unix 도메인 소켓 클라이언트 코드 (client.c)

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

### 6.3. 인터넷 소켓 에코 서버 코드 (inet_server.c)

```c
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define PORTNUM 9000
#define BUFFER_SIZE 256

int main() {
    char buf[BUFFER_SIZE];
    struct sockaddr_in sin, cli;
    int sd, ns, clientlen = sizeof(cli);

    // 소켓 생성
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 서버 주소 구조체 초기화
    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = INADDR_ANY;  // 모든 인터페이스에서 연결 수락

    // 포트 바인딩
    if (bind(sd, (struct sockaddr *)&sin, sizeof(sin))) {
        perror("bind");
        exit(1);
    }

    // 연결 대기 모드로 전환
    if (listen(sd, 5)) {
        perror("listen");
        exit(1);
    }

    printf("에코 서버가 시작되었습니다. 포트: %d\n", PORTNUM);
    
    // 클라이언트 연결 수락
    if ((ns = accept(sd, (struct sockaddr *)&cli, &clientlen)) == -1) {
        perror("accept");
        exit(1);
    }

    printf("클라이언트 연결됨: %s\n", inet_ntoa(cli.sin_addr));
    
    // 클라이언트로부터 메시지를 받아서 그대로 다시 보내는 에코 서버
    while (1) {
        memset(buf, 0, BUFFER_SIZE);
        int recv_len = recv(ns, buf, BUFFER_SIZE, 0);
        
        if (recv_len <= 0) {
            printf("클라이언트 연결 종료\n");
            break;
        }
        
        printf("수신 메시지: %s\n", buf);
        
        // 받은 메시지를 그대로 다시 전송
        if (send(ns, buf, recv_len, 0) == -1) {
            perror("send");
            break;
        }
        
        printf("메시지 에코 완료\n");
    }
    
    close(ns);
    close(sd);
    return 0;
}
```

### 6.4. 인터넷 소켓 에코 클라이언트 코드 (inet_client.c)

```c
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PORTNUM 9000
#define BUFFER_SIZE 256

int main() {
    int sd;
    char send_buf[BUFFER_SIZE];
    char recv_buf[BUFFER_SIZE];
    struct sockaddr_in sin;

    // 소켓 생성
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // 서버 주소 구조체 초기화
    memset((char *)&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORTNUM);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");  // localhost IP

    // 서버에 연결
    if (connect(sd, (struct sockaddr *)&sin, sizeof(sin))) {
        perror("connect");
        exit(1);
    }

    printf("서버에 연결되었습니다.\n");
    
    // 대화형 인터페이스로 메시지 송수신
    while (1) {
        printf("전송할 메시지 (종료하려면 'exit' 입력): ");
        fgets(send_buf, BUFFER_SIZE, stdin);
        
        // 개행 문자 제거
        send_buf[strcspn(send_buf, "\n")] = '\0';
        
        // 'exit' 입력 시 종료
        if (strcmp(send_buf, "exit") == 0) {
            printf("프로그램을 종료합니다.\n");
            break;
        }
        
        // 메시지 전송
        if (send(sd, send_buf, strlen(send_buf), 0) == -1) {
            perror("send");
            break;
        }
        
        // 서버로부터 에코된 메시지 수신
        memset(recv_buf, 0, BUFFER_SIZE);
        if (recv(sd, recv_buf, BUFFER_SIZE, 0) == -1) {
            perror("recv");
            break;
        }
        
        printf("서버로부터 에코: %s\n", recv_buf);
    }
    
    close(sd);
    return 0;
}
```

## 7. 빌드 및 실행 방법

### 7.1. 빌드

프로젝트를 빌드하기 위해 다음 명령어를 실행합니다:

```bash
make
```

이 명령은 모든 소스 파일을 컴파일하여 다음 실행 파일을 생성합니다:
- `unix_client`: Unix 도메인 소켓 클라이언트
- `unix_server`: Unix 도메인 소켓 서버
- `inet_client`: 인터넷 소켓 에코 클라이언트
- `inet_server`: 인터넷 소켓 에코 서버

### 7.2. Unix 도메인 소켓 실행

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

### 7.3. 인터넷 소켓 에코 서버 실행

1. 먼저 에코 서버를 실행합니다:

```bash
./inet_server
```

서버는 "에코 서버가 시작되었습니다. 포트: 9000" 메시지를 출력하고 클라이언트의 연결을 기다립니다.

2. 다른 터미널에서 에코 클라이언트를 실행합니다:

```bash
./inet_client
```

3. 클라이언트 프롬프트에서 메시지를 입력하면 서버로 전송되고, 서버는 이를 그대로 다시 클라이언트에게 반환합니다.

4. 클라이언트에서 "exit"를 입력하면 프로그램이 종료됩니다.

## 8. 소켓 통신의 특징

### 8.1. Unix 도메인 소켓의 특징

1. **로컬 통신**: 같은 호스트 내 프로세스 간 통신에 사용됩니다.
2. **파일 시스템 기반**: 소켓은 파일 시스템에 경로로 표현됩니다.
3. **효율성**: 네트워크 스택을 거치지 않아 TCP/IP 소켓보다 더 효율적입니다.
4. **보안**: 로컬 시스템 내에서만 통신하므로 네트워크 보안 위험이 없습니다.
5. **신뢰성**: 데이터 손실이나 순서 변경 없이 안정적인 데이터 전송이 가능합니다.

### 8.2. 인터넷 소켓의 특징

1. **네트워크 통신**: 다른 호스트와 통신 가능합니다.
2. **IP 주소와 포트**: 통신 대상을 IP 주소와 포트 번호로 식별합니다.
3. **프로토콜 사용**: TCP/IP 등의 네트워크 프로토콜을 사용합니다.
4. **범용성**: 로컬 및 원격 통신에 모두 사용 가능합니다.
5. **보안 고려**: 네트워크를 통한 통신이므로 보안에 주의해야 합니다.

## 9. 에코 서버의 특징과 활용

에코 서버는 클라이언트가 보낸 메시지를 그대로 다시 반환하는 단순한 형태의 서버입니다. 주요 특징과 활용은 다음과 같습니다:

1. **네트워크 테스트**: 네트워크 연결 및 통신 테스트에 유용합니다.
2. **디버깅 도구**: 클라이언트-서버 통신 문제를 진단하는 도구로 활용됩니다.
3. **교육용**: 소켓 프로그래밍의 기본 개념을 이해하는 데 도움이 됩니다.
4. **프로토콜 테스트**: 새로운 네트워크 프로토콜을 테스트하는 데 사용될 수 있습니다.
5. **응답 시간 측정**: 네트워크 지연 시간을 측정하는 데 활용될 수 있습니다.

## 10. 결론

본 프로젝트에서는 두 가지 타입의 소켓 통신 모델을 구현했습니다:

1. Unix 도메인 소켓을 이용한 로컬 프로세스 간 통신
2. 인터넷 소켓을 이용한 에코 서버-클라이언트 통신

이를 통해 소켓 프로그래밍의 기본 원리와 다양한 형태의 통신 방법을 학습할 수 있었습니다. 특히 Unix 도메인 소켓의 효율성과 인터넷 소켓의 범용성을 비교하고, 에코 서버의 구현을 통해 양방향 통신의 기본 원리를 이해할 수 있었습니다.

## 11. 참고 자료

- Stevens, W. R. (2003). UNIX Network Programming, Volume 1: The Sockets Networking API (3rd ed.). Addison-Wesley.
- Kerrisk, M. (2010). The Linux Programming Interface. No Starch Press.
- Beej's Guide to Network Programming (https://beej.us/guide/bgnet/)
- 리눅스 매뉴얼 페이지: socket(2), bind(2), listen(2), accept(2), connect(2), send(2), recv(2), close(2) 