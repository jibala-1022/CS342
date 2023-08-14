#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "base64.c"

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("[-] Usage: %s <server_ip_address> <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    char *server_ip = argv[1];

    int client_socket;
    struct sockaddr_in server_addr;
    char *buffer = (char *) malloc(BUFFER_SIZE * sizeof(char));
    char *encoded_buffer = (char *) malloc(2 * BUFFER_SIZE * sizeof(char));

    // Create a socket for communication
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("[-] Socket creation failed\n");
        return 1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("[-] Connection failed\n");
        return 1;
    }

    // Receive initial response from the server
    recv(client_socket, buffer, strlen("0"), 0);
    if (buffer[0] == '0') {
        perror("[-] Server capacity reached\n");
        close(client_socket);
        return 1;
    }
    printf("[+] Connected to server...\n");

    while (1) {
        printf("[*] Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Encode the message using Base64
        encoded_buffer = base64_encode(buffer);

        // Send the encoded message to the server
        if (send(client_socket, encoded_buffer, strlen(encoded_buffer), 0) == -1) {
            perror("[-] Sending failed\n");
            break;
        }

        // Check if the client wants to exit
        if (strcmp(buffer, "/exit\n") == 0) {
            printf("[-] Connection closed by client\n");
            break;
        }

        // Clear the encoded buffer for receiving
        memset(encoded_buffer, 0, 2 * BUFFER_SIZE);

        // Receive the server's response
        if (recv(client_socket, encoded_buffer, 2 * BUFFER_SIZE, 0) <= 0) {
            perror("[-] Receiving failed\n");
            break;
        }

        // Decode the received message using Base64
        buffer = base64_decode(encoded_buffer);

        // Print the server's response
        printf("[*] %s\n", buffer);
    }

    // Free allocated memory
    free(buffer);
    free(encoded_buffer);
    
    // Close the client socket
    close(client_socket);

    printf("[-] Client closed\n");

    return 0;
}
