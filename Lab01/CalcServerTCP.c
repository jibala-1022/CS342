#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ipc.h>
#include "calculator.c"

#define MAX_CLIENTS 5
#define BUFFER_SIZE 1024

// Function to handle a connected client
void handle_client(int client_socket) {
    int bytes_received;
    char buffer[BUFFER_SIZE];

    // Continuously receive and process client data
    while (1) {
        // Receive data from the client
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }

        // Print received expression
        printf("[>] Expression: %s", buffer);

        // Evaluate the expression using the calculator_evaluate function
        char *result = calculator_evaluate(buffer);

        // Print calculated result
        printf("[<] Result: %s\n", result);
    
        // Send the result back to the client
        if (send(client_socket, result, strlen(result), 0) == -1) {
            perror("[-] Sending failed\n");
            break;
        }

        free(result); // Free the allocated memory for the result
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("[-] Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create a socket for TCP communication
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("[-] Socket creation failed\n");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("[-] Binding failed\n");
        return 1;
    }

    // Listen for incoming client connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("[-] Listening failed\n");
        return 1;
    }

    printf("[+] TCP Server listening on port %d...\n\n", port);

    // Continuously accept and handle client connections
    while (1) {
        // Accept a client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            perror("[-] Accepting connection failed\n");
            continue;
        } else {
            // Print client connection information
            printf("[+] Client connected - %s : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Handle the client in a separate function
            handle_client(client_socket);

            // Close the client socket and print status
            close(client_socket);
            printf("Client closed\n");
        }
    }

    // Close the server socket and print status
    close(server_socket);
    printf("[-] Server closed\n");
    return 0;
}
