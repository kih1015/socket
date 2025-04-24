#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#define PORT 8080
#define BACKLOG 16
#define BUF_SZ 4096
#define WEBROOT "./www"

static const char *mime_type(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (!strcmp(dot, ".html")) return "text/html";
    if (!strcmp(dot, ".css")) return "text/css";
    if (!strcmp(dot, ".js")) return "application/javascript";
    if (!strcmp(dot, ".png")) return "image/png";
    if (!strcmp(dot, ".jpg") || !strcmp(dot, ".jpeg")) return "image/jpeg";
    return "application/octet-stream";
}

static void send_error(int c, int code, const char *msg) {
    char body[128];
    int n = snprintf(body, sizeof body, 
        "<html><h1>%d %s</h1></html>", code, msg);
    dprintf(c,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n\r\n%s",
        code, msg, n, body);
}

static void serve_client(int c) {
    char buf[BUF_SZ] = {0};
    int len = read(c, buf, sizeof buf - 1);
    if (len <= 0) return;

    char method[8], path_raw[1024];
    if (sscanf(buf, "%7s %1023s", method, path_raw) != 2) {
        send_error(c, 400, "Bad Request");
        return;
    }

    if (strcmp(method, "GET")) {
        send_error(c, 405, "Method Not Allowed");
        return;
    }
    if (strstr(path_raw, "..")) {
        send_error(c, 400, "Bad Request");
        return;
    }

    const char *path_req = (*path_raw == '/') ? path_raw + 1 : path_raw;
    if (*path_req == '\0') path_req = "/index.html";
        
    char path_full[PATH_MAX];
    snprintf(path_full, sizeof path_full, "%s%s", WEBROOT, path_req);

    int fd = open(path_full, O_RDONLY);
    if (fd < 0) {
        send_error(c, 404, "Not Found");
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        send_error(c, 404, "Not Found");
        return;
    }

    const char *type = mime_type(path_full);
    dprintf(c,
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: %ld\r\n"
        "Content-Type: %s\r\n\r\n",
        (long)st.st_size, type);

    off_t offset = 0;
    while (offset < st.st_size) {
        ssize_t sent = sendfile(c, fd, &offset, st.st_size - offset);
        if (sent <= 0) break;
    }
    close(fd);
}

int main(void) {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;

    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(PORT),
        .sin_addr = { INADDR_ANY }
    };
    bind(s, (struct sockaddr *)&addr, sizeof addr);
    listen(s, BACKLOG);

    printf("Serving %s on http://0.0.0.0:%d\n", WEBROOT, PORT);

    while (1) {
        int c = accept(s, NULL, NULL);
        if (c < 0) continue;
        
        pid_t pid = fork();
        if (pid == 0) {
            close(s);
            serve_client(c);
            close(c);
            exit(0);
        }
        close(c);
    }
}
