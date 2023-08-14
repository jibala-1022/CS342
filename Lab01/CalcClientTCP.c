#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 3) {
        printf("[-] Usage: %s <server_ip_address> <port>\n", argv[0]);
        return 1;
    }

    // Extract the server IP address and port number from the command line arguments
    int port = atoi(argv[2]);
    char *server_ip = argv[1];

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create a socket for TCP communication
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("[-] Socket creation failed\n");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    // Set the server IP address and port to connect to
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("[-] Connection failed\n");
        return 1;
    }

    printf("[+] Connected to server...\n\n");

    // Continuously read user input and communicate with the server
    while (1) {
        printf("[ ] Format of expression: <operand_1> <operation> <operand_2>\n");
        printf("[ ] Valid operands: int or float\n");
        printf("[ ] Valid operations: +, -, *, /, ^\n");
        printf("[ ] Enter -1 to quit\n");
        printf("[<] Enter the expression: ");
        
        // Read user input
        fgets(buffer, BUFFER_SIZE, stdin);

        // Check if the user wants to exit
        if (strcmp(buffer, "-1\n") == 0) {
            printf("[-] Connection closed by client\n");
            break;
        }

        // Send the expression to the server
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("[-] Sending failed\n");
            break;
        }

        // Clear the buffer for receiving the result
        memset(buffer, 0, BUFFER_SIZE);

        // Receive the result from the server
        if (recv(client_socket, buffer, BUFFER_SIZE, 0) <= 0) {
            perror("[-] Receiving failed\n");
            break;
        }

        // Display the calculated result
        printf("[>] Result: %s\n\n", buffer);
    }

    // Close the client socket
    close(client_socket);

    printf("[-] Client closed\n");

    return 0;
}
