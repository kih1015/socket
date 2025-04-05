#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int main() {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    socklen_t server_addr_len;
    
    // 1. Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // 2. Initialize server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Localhost for testing
    
    printf("UDP client started (server port: %d)\n", PORT);
    
    while (1) {
        printf("Enter a message to send (type 'q' to quit): ");
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            // Handle EOF (Ctrl+D, etc.)
            break;
        }
        
        // Check for quit command
        if (buffer[0] == 'q') {
            break;
        }
        
        // 3. Send message to server
        if (sendto(sockfd, buffer, strlen(buffer), 0,
                  (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("sendto failed");
            break;
        }
        
        // 4. Receive echo from server
        server_addr_len = sizeof(server_addr);
        int received_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
                                  (struct sockaddr*)&server_addr, &server_addr_len);
        if (received_len < 0) {
            perror("recvfrom failed");
            break;
        }
        
        buffer[received_len] = '\0';
        printf("Echo from server: %s\n", buffer);
    }
    
    close(sockfd);
    return 0;
}
