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

    // Create a socket for UDP communication
    client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("[-] Socket creation failed\n");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    printf("[+] Connected to server...\n\n");

    while (1) {
        printf("[|] Format of expression: <operand_1> <operation> <operand_2>\n");
        printf("[|] Valid operands: int or float\n");
        printf("[|] Valid operations: +, -, *, /, ^\n");
        printf("[|] Enter -1 to quit\n");
        printf("[<] Enter the expression: ");
        fgets(buffer, BUFFER_SIZE, stdin);

        // Check if the user wants to quit
        if (strcmp(buffer, "-1\n") == 0) {
            printf("[-] Connection closed by client\n");
            break;
        }

        // Send the expression to the server using the client socket
        if (sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("[-] Sending failed\n");
            break;
        }

        // Clear the buffer for receiving data
        memset(buffer, 0, BUFFER_SIZE);

        socklen_t server_addr_len = sizeof(server_addr);
        // Receive the result from the server
        if (recvfrom(client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&server_addr, &server_addr_len) <= 0) {
            perror("[-] Receiving failed\n");
            break;
        }

        // Display the received result
        printf("[>] Result: %s\n\n", buffer);
    }

    // Close the client socket
    close(client_socket);

    printf("[-] Client closed\n");

    return 0;
}
