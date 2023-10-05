// gcc -pthread q3_client.c -o client && ./client <server_ip_addr> <server_port>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <signal.h>

#define BUFFER_SIZE 1024

// Structure to hold thread arguments
typedef struct ThreadArgs {
    int client_socket;
    int* server_status;
} ThreadArgs;

// Function to receive messages from the server
void* receive(void* args) {
    // Cast the arguments to the correct type
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    int client_socket = threadArgs->client_socket;
    int* server_status = threadArgs->server_status;

    char buffer[BUFFER_SIZE];
    int bytes_received;

    while (1) {
        // Clear the buffer+
        memset(buffer, 0, BUFFER_SIZE);
        
        // Receive a message from the server
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        
        // Check for receive errors or connection termination
        if (bytes_received <= 0) {
            *server_status = 0; // Set server status to indicate failure
            break;
        }
        buffer[bytes_received] = '\0';

        // Process received message based on the first character
        if (buffer[0] == '0') {
            // Message sent successfully
        } else if (buffer[0] == '1') {
            printf("Invalid message format\n");
        } else if (buffer[0] == '2') {
            printf("Invalid recipient\n");
        } else if (buffer[0] == '3') {
            printf("%s\n", buffer + 1); // Print the message (excluding the type code)
        } else {
            printf("Garbage message\n");
        }
    }
    printf("Server terminated\n");
    kill(getpid(), SIGTERM);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip_address> <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    char* server_ip = argv[1];

    char buffer[BUFFER_SIZE];

    int bytes_received;
    int logged = 0;
    int server_status = 1;

    // Create a socket for communication
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        printf("Socket creation failed\n");
        return 1;
    }

    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("Socket connection failed\n");
        return 1;
    }

    // Receive initial response from the server
    memset(buffer, 0, BUFFER_SIZE);
    bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received <= 0) {
        printf("Receiving failed - connection\n");
        close(client_socket);
        return 1;
    }
    if (buffer[0] == '1') {
        printf("Server capacity reached\n");
        close(client_socket);
        return 1;
    }

    printf("Connected to server...\n");

    while (logged == 0) {
        printf("Enter username: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        if (buffer[0] == '\n') {
            continue;
        }

        // Exit command
        if (strcmp(buffer, "/exit\n") == 0) {
            printf("Connection closed by client\n");
            return 1;
        }

        // Send the username to the server for authentication
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            printf("Sending failed - auth\n");
            return 1;
        }

        // Receive the response from the server
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Receiving failed - auth\nServer terminated\n");
            return 1;
        }
        buffer[bytes_received] = '\0';

        // Check the response from the server
        if (buffer[0] == '0') {
            printf("Joined the chat\n- Use '@<username>' to send private message\n- Use '/broadcast' to broadcast to all users\n- Use '/exit' to quit\n");
            logged = 1;
            break;
        } else {
            printf("Username already exists\n");
        }
    }

    // Prepare thread arguments for the receive thread
    ThreadArgs* threadArgs = (ThreadArgs*)malloc(sizeof(ThreadArgs));
    threadArgs->client_socket = client_socket;
    threadArgs->server_status = &server_status;

    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive, (void*)threadArgs) != 0) {
        printf("pthread create failed\n");
        return 1;
    }

    // Set stdin to non-blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    while (logged == 1) {
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        
        // Exit command
        if (strcmp(buffer, "/exit\n") == 0) {
            printf("Connection closed by client\n");
            return 1;
        }

        if (server_status == 1 && send(client_socket, buffer, strlen(buffer), 0) == -1) {
            printf("Sending failed - chat\n");
            break;
        }
    }

    if (pthread_join(receive_thread, NULL) != 0) {
        printf("pthread join failed\n");
    }

    // Close the client socket
    close(client_socket);
    free(threadArgs);

    printf("Client closed\n");

    return 0;
}
