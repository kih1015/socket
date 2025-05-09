/*
 * Simple Chat Server in C
 * Requirements:
 * - Support multiple simultaneous clients
 * - Prompt clients for nickname upon connection
 * - Manage client list until logout or disconnection
 * - Broadcast messages to all connected clients
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 12345
#define MAX_CLIENTS FD_SETSIZE
#define BUF_SIZE 1024
#define NAME_LEN 32

int main() {
    int listener, newfd;
    struct sockaddr_in server_addr, client_addr;
    fd_set master_set, read_fds;
    int fdmax, i;
    int client_fds[MAX_CLIENTS];
    char client_names[MAX_CLIENTS][NAME_LEN];
    socklen_t addrlen;
    char buf[BUF_SIZE];
    int nbytes;

    // Initialize client_fds to -1
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
        client_names[i][0] = '\0';
    }

    // Create listener socket
    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Allow address reuse
    int opt = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(listener, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Init fd sets
    FD_ZERO(&master_set);
    FD_ZERO(&read_fds);
    FD_SET(listener, &master_set);
    fdmax = listener;

    printf("Chat server started on port %d\n", PORT);

    // Main loop
    while (1) {
        read_fds = master_set;
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        // Check all fds
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == listener) {
                    // New connection
                    addrlen = sizeof(client_addr);
                    newfd = accept(listener, (struct sockaddr*)&client_addr, &addrlen);
                    if (newfd < 0) {
                        perror("accept");
                        continue;
                    }
                    // Add to client_fds
                    int j;
                    for (j = 0; j < MAX_CLIENTS; j++) {
                        if (client_fds[j] < 0) {
                            client_fds[j] = newfd;
                            break;
                        }
                    }
                    if (j == MAX_CLIENTS) {
                        fprintf(stderr, "Max clients reached. Connection refused.\n");
                        close(newfd);
                    } else {
                        FD_SET(newfd, &master_set);
                        if (newfd > fdmax) fdmax = newfd;
                        // Prompt for nickname
                        send(newfd, "Enter your nickname: ", 21, 0);
                        // Receive nickname
                        if ((nbytes = recv(newfd, buf, NAME_LEN-1, 0)) <= 0) {
                            close(newfd);
                            FD_CLR(newfd, &master_set);
                            client_fds[j] = -1;
                        } else {
                            buf[nbytes] = '\0';
                            // remove newline/cr
                            buf[strcspn(buf, "\r\n")] = '\0';
                            strncpy(client_names[j], buf, NAME_LEN);
                            // Welcome message
                            snprintf(buf, sizeof(buf), "%s has joined.\n", client_names[j]);
                            // Broadcast join
                            for (int k = 0; k < MAX_CLIENTS; k++) {
                                if (client_fds[k] >= 0 && k != j) {
                                    send(client_fds[k], buf, strlen(buf), 0);
                                }
                            }
                            printf("%s connected from %s:%d\n", client_names[j], 
                                   inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                        }
                    }
                } else {
                    // Data from a client
                    int idx;
                    // find index
                    for (idx = 0; idx < MAX_CLIENTS; idx++) {
                        if (client_fds[idx] == i) break;
                    }
                    if ((nbytes = recv(i, buf, sizeof(buf), 0)) <= 0) {
                        // Connection closed
                        close(i);
                        FD_CLR(i, &master_set);
                        if (idx < MAX_CLIENTS) {
                            snprintf(buf, sizeof(buf), "%s has left.\n", client_names[idx]);
                            // Broadcast leave
                            for (int k = 0; k < MAX_CLIENTS; k++) {
                                if (client_fds[k] >= 0) send(client_fds[k], buf, strlen(buf), 0);
                            }
                            client_fds[idx] = -1;
                            client_names[idx][0] = '\0';
                        }
                    } else {
                        buf[nbytes] = '\0';
                        // remove newline
                        buf[strcspn(buf, "\r\n")] = '\0';
                        // Handle quit command
                        if (strcmp(buf, "/quit") == 0) {
                            close(i);
                            FD_CLR(i, &master_set);
                            if (idx < MAX_CLIENTS) {
                                snprintf(buf, sizeof(buf), "%s has left.\n", client_names[idx]);
                                for (int k = 0; k < MAX_CLIENTS; k++) {
                                    if (client_fds[k] >= 0) send(client_fds[k], buf, strlen(buf), 0);
                                }
                                client_fds[idx] = -1;
                                client_names[idx][0] = '\0';
                            }
                        } else {
                            // Broadcast message
                            snprintf(msg, sizeof(msg), "%s: %s\n", client_names[idx], buf);
                            for (int k = 0; k < MAX_CLIENTS; k++) {
                                if (client_fds[k] >= 0 && client_fds[k] != i) {
                                    send(client_fds[k], msg, strlen(msg), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}
