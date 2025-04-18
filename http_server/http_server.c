#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define PORT 8080
#define REPLY
  "HTTP/1.1 200 OK\r\n" \
  "Content-Type: text/html\r\n" \
  "Content-Length: 13\r\n" \
  "\r\nHellor, World!" \

int main(void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr = { INADDR_ANY }
    };

    bind(s, (struct sockaddr*)&addr, sizeof(addr));
    listen(s, 16);

    while (1) {
        int c = accept(s, NULL, NULL);
        char buf[1024];
        read(c, buf, sizeof buf);
        write(c, REPLY, sizeof REPLY - 1);
        close(c);
    }
}
