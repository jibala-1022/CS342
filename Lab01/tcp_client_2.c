#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

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
    char buffer[BUFFER_SIZE];
    
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
    printf("[+] Connected to server...\n\n");

    while (1) {
        printf("[*] Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Send the message to the server
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("[-] Sending failed\n");
            break;
        }

        // Check if the client wants to exit
        if (strcmp(buffer, "/exit\n") == 0) {
            printf("[-] Connection closed by client\n");
            break;
        }

        // Clear the buffer for receiving
        memset(buffer, 0, BUFFER_SIZE);

        // Receive the server's response
        if (recv(client_socket, buffer, BUFFER_SIZE, 0) <= 0) {
            perror("[-] Receiving failed\n");
            break;
        }

        // Print the server's response
        printf("[*] %s\n", buffer);
    }
    
    // Close the client socket
    close(client_socket);

    printf("[-] Client closed\n");

    return 0;
}
