#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "calculator.c"

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    // Check if the correct number of arguments is provided
    if (argc != 2) {
        printf("[-] Usage: %s <port>\n", argv[0]);
        return 1;
    }

    // Extract the port number from the command line argument
    int port = atoi(argv[1]);

    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create a socket for UDP communication
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("[-] Socket creation failed\n");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Set the server port to bind to
    server_addr.sin_port = htons(port);

    // Bind the server socket to the specified address and port
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("[-] Binding failed\n");
        return 1;
    }

    printf("[+] UDP Server listening on port %d...\n\n", port);

    char buffer[BUFFER_SIZE];

    while (1) {
        // Receive data from clients
        int bytes_received = recvfrom(server_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (bytes_received == -1) {
            perror("[-] Receiving failed\n");
            continue;
        }

        buffer[bytes_received] = '\0';
        printf("[>] Expression: %s", buffer);

        // Evaluate the expression using the calculator function
        char *result = calculator_evaluate(buffer);

        // Send the result back to the client
        if (sendto(server_socket, result, strlen(result), 0, (struct sockaddr *)&client_addr, client_addr_len) == -1) {
            perror("[-] Sending failed\n");
        } else {
            printf("[<] Result: %s\n", result);
        }

        // Free the dynamically allocated memory for the result
        free(result);
    }

    // Close the server socket
    close(server_socket);
    printf("[-] Server closed\n");
    return 0;
}
