#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "logger.h"
#include "monitor.h"

#define PORT 8080
#define MAX_CLIENTS 100
#define MAX_EVENTS 100
#define BUFFER_SIZE 1024
#define MAX_NAME_LEN 32

int client_fds[MAX_CLIENTS];
char client_names[MAX_CLIENTS][MAX_NAME_LEN]; // Array to store usernames
int client_count = 0;
pthread_mutex_t lock;
int stop = 0;
int server_fd;

void handle_signal(int sig) {
    stop = 1;
    log_message("Signal %d received. Shutting down server.", sig);
}

// Broadcast message with sender's username as prefix.
void broadcast_message(int sender_fd, char *message) {
    char send_buffer[BUFFER_SIZE + MAX_NAME_LEN + 10]; // extra space for prefix
    int sender_index = -1;
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (client_fds[i] == sender_fd) {
            sender_index = i;
            break;
        }
    }
    // If we found a valid index, prefix with username
    if (sender_index != -1) {
        snprintf(send_buffer, sizeof(send_buffer), "[%s]: %s", client_names[sender_index], message);
    } else {
        // Fallback: no username found, just send the message
        strncpy(send_buffer, message, sizeof(send_buffer)-1);
        send_buffer[sizeof(send_buffer)-1] = '\0';
    }
    // Broadcast to all other clients.
    for (int i = 0; i < client_count; i++) {
        if (client_fds[i] != sender_fd) {
            send(client_fds[i], send_buffer, strlen(send_buffer), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

void *handle_client(void *arg) {
    int client_fd = *(int *)arg;
    char buffer[BUFFER_SIZE];
    int bytes_read;
    int initialized = 0;  // To check if we received the username.

    while (1) {
        bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            // If not initialized, treat the first message as the username.
            if (!initialized) {
                pthread_mutex_lock(&lock);
                // Save the username (limit to MAX_NAME_LEN-1 characters)
                strncpy(client_names[client_count - 1], buffer, MAX_NAME_LEN - 1);
                client_names[client_count - 1][MAX_NAME_LEN - 1] = '\0';
                initialized = 1;
                pthread_mutex_unlock(&lock);
                log_message("Client %d set username to: %s", client_fd, client_names[client_count - 1]);
                continue;
            }
            log_message("Message from client[%d]: %s", client_fd, buffer);
            printf("Client[%d]: %s", client_fd, buffer);
            broadcast_message(client_fd, buffer);
        } else if (bytes_read == 0) {
            log_message("Client %d disconnected.", client_fd);
            printf("Client %d disconnected\n", client_fd);
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                log_message("Error receiving from client[%d]: %s", client_fd, strerror(errno));
                perror("Error receiving from client");
                break;
            }
        }
    }

    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; i++) {
        if (client_fds[i] == client_fd) {
            // Replace this client with the last one in the list.
            client_fds[i] = client_fds[client_count - 1];
            strncpy(client_names[i], client_names[client_count - 1], MAX_NAME_LEN);
            client_count--;
            update_client_count(client_count);
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    close(client_fd);
    free(arg);
    return NULL;
}

int set_non_blocking(int sock) {
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    return 0;
}

int main() {
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    int new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int epoll_fd;
    struct epoll_event event, events[MAX_EVENTS];
    int bind_status;

    pthread_mutex_init(&lock, NULL);

    // Initialize logger with filename "chat.log"
    init_logger("chat.log");

    // Create server socket.
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        log_message("Error creating socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind_status = bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    if (bind_status < 0) {
        log_message("Error binding socket: %s", strerror(errno));
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        log_message("Error listening on socket: %s", strerror(errno));
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    log_message("Server listening on port %d", PORT);
    printf("Server listening on port %d\n", PORT);

    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        log_message("Error creating epoll: %s", strerror(errno));
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    set_non_blocking(server_fd);
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event);

    // Start the monitoring thread.
    start_monitoring();

    while (!stop) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < num_events; i++) {
            if (events[i].data.fd == server_fd) {
                // New client connection request.
                new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
                if (new_socket < 0) {
                    log_message("Error accepting client: %s", strerror(errno));
                    continue;
                }

                set_non_blocking(new_socket);
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = new_socket;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &event);

                pthread_mutex_lock(&lock);
                client_fds[client_count] = new_socket;
                // Assign a default username (empty) for now.
                strcpy(client_names[client_count], "Unknown");
                client_count++;
                update_client_count(client_count);
                pthread_mutex_unlock(&lock);

                log_message("New client connected: %d", new_socket);
                printf("New client connected: %d\n", new_socket);

                // Create a new thread to handle client communication.
                pthread_t thread;
                int *new_sock = malloc(sizeof(int));
                if (new_sock == NULL) {
                    log_message("Memory allocation failed for client socket");
                    close(new_socket);
                    continue;
                }
                *new_sock = new_socket;
                pthread_create(&thread, NULL, handle_client, (void *)new_sock);
                pthread_detach(thread);
            }
        }
    }

    log_message("Shutting down server...");
    printf("\nShutting down server...\n");

    close(server_fd);
    for (int i = 0; i < client_count; i++) {
        close(client_fds[i]);
    }
    pthread_mutex_destroy(&lock);
    close_logger();

    return 0;
}
