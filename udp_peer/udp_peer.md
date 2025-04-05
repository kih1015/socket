# UDP P2P 통신 프로그램

이 프로그램은 UDP 프로토콜을 사용하여 두 개의 피어 간에 양방향 통신을 구현한 P2P(Peer-to-Peer) 통신 프로그램입니다.

## 주요 특징

- 서버/클라이언트 구분 없이 동작하는 P2P 구조
- `fork()`를 사용하여 데이터 수신과 송신을 별도의 프로세스로 분리
- UDP 프로토콜을 사용한 실시간 양방향 통신
- 간단한 명령행 인터페이스

## 컴파일 방법

```bash
make
```

## 실행 방법

```bash
./udp_peer <local_port> <remote_ip> <remote_port>
```

### 매개변수 설명
- `local_port`: 데이터를 수신할 로컬 포트 번호
- `remote_ip`: 데이터를 전송할 원격 호스트의 IP 주소
- `remote_port`: 데이터를 전송할 원격 호스트의 포트 번호

## 사용 예시

두 개의 터미널에서 다음과 같이 실행하여 서로 통신할 수 있습니다:

터미널 1:
```bash
./udp_peer 5000 127.0.0.1 5001
```

터미널 2:
```bash
./udp_peer 5001 127.0.0.1 5000
```

## 프로그램 동작 방식

1. 프로그램이 시작되면 지정된 로컬 포트에 UDP 소켓을 생성하고 바인딩합니다.
2. `fork()`를 호출하여 프로세스를 두 개로 분리합니다:
   - 자식 프로세스: 데이터 수신 담당
   - 부모 프로세스: 데이터 송신 담당
3. 각 프로세스는 독립적으로 동작하며, 실시간으로 메시지를 주고받을 수 있습니다.

## 구현 설명

### 소켓 초기화 및 설정
```c
int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
struct sockaddr_in local_addr;
memset(&local_addr, 0, sizeof(local_addr));
local_addr.sin_family = AF_INET;
local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
local_addr.sin_port = htons(local_port);
bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr));
```

### 프로세스 분리
```c
pid_t pid = fork();
if (pid == 0) {
    // 자식 프로세스: 데이터 수신
    receive_data(sock_fd);
} else {
    // 부모 프로세스: 데이터 송신
    send_data(sock_fd, remote_ip, remote_port);
}
```

### 데이터 수신 함수
```c
void receive_data(int sock_fd) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int recv_len = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0,
                               (struct sockaddr *)&client_addr, &client_len);
        
        if (recv_len > 0) {
            printf("수신: %s\n", buffer);
        }
    }
}
```

### 데이터 송신 함수
```c
void send_data(int sock_fd, const char *remote_ip, int remote_port) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(remote_port);
    server_addr.sin_addr.s_addr = inet_addr(remote_ip);

    while (1) {
        printf("메시지 입력: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        if (strlen(buffer) > 0) {
            sendto(sock_fd, buffer, strlen(buffer), 0,
                   (struct sockaddr *)&server_addr, sizeof(server_addr));
        }
    }
}
```

### 주요 구현 특징
1. **UDP 소켓 사용**: 비연결형 통신을 위해 UDP 프로토콜 사용
2. **프로세스 분리**: `fork()`를 사용하여 수신과 송신을 별도 프로세스로 분리
3. **무한 루프**: 각 프로세스는 무한 루프로 실행되어 지속적인 통신 가능
4. **버퍼 관리**: 메모리 누수 방지를 위한 적절한 버퍼 초기화
5. **에러 처리**: 소켓 생성, 바인딩 등의 주요 작업에 대한 에러 처리

## 주의사항

- 프로그램을 종료하려면 Ctrl+C를 사용하세요.
- 로컬 포트는 이미 사용 중인 포트를 사용하지 않도록 주의하세요.
- 원격 IP 주소는 올바른 형식이어야 합니다 (예: 127.0.0.1, 192.168.1.100 등). 