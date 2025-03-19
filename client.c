#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024
#define MAX_NAME_LEN 32

int sock_fd;

void handle_signal(int sig) {
    printf("\nReceived signal %d. Closing client socket...\n", sig);
    close(sock_fd);
    exit(0);
}

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    while ((bytes_read = recv(sock_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("\n%s", buffer);
        printf("\n[YOU]: ");
        fflush(stdout);
    }
    
    if (bytes_read == 0) {
        printf("\nServer disconnected.\n");
    } else {
        perror("Error receiving message from server");
    }
    
    close(sock_fd);
    exit(0);
}

int main() {
    struct sockaddr_in server_addr;
    pthread_t recv_thread;
    char username[MAX_NAME_LEN];
    char buffer[BUFFER_SIZE];

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Prompt for username
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    // Remove newline if present
    username[strcspn(username, "\n")] = '\0';

    // Create socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);
    
    // Send username as the first message
    send(sock_fd, username, strlen(username), 0);
    
    // Create a thread to receive messages from the server
    if (pthread_create(&recv_thread, NULL, receive_messages, NULL) != 0) {
        perror("Error creating receive thread");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    pthread_detach(recv_thread);
    
    // Loop to send messages to the server
    while (1) {
        printf("[YOU]: ");
        fgets(buffer, sizeof(buffer), stdin);
        if (send(sock_fd, buffer, strlen(buffer), 0) < 0) {
            perror("Error sending message");
            break;
        }
    }
    
    close(sock_fd);
    return 0;
}
