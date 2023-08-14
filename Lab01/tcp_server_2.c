#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024

static int *client_count; // Pointer to shared memory for client count

// Function to handle communication with a client
void handle_client(int client_socket, int client_id) {
    int bytes_received;
    char buffer[BUFFER_SIZE];
    while (1) {
        // Receive data from the client
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            // If no bytes received or an error occurred, break the loop
            break;
        }

        // Print the received message from the client
        printf("\n[*] Client %d: %s", client_id, buffer);

        // For simplicity, let's just send back the same data as a response
        printf("[*] Reply to Client %d: ", client_id);
        fgets(buffer, BUFFER_SIZE, stdin);

        // Send the response back to the client
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("[-] Sending failed\n");
            break;
        }
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

    // Create the server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("[-] Socket creation failed\n");
        return 1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the server socket to the specified port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("[-] Binding failed\n");
        return 1;
    }

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("[-] Listening failed\n");
        return 1;
    }

    printf("[+] Server listening on port %d...\n", port);

    // Create shared memory segment for client count
    int shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("[-] Shared memory creation failed\n");
        return 1;
    }

    // Attach to the shared memory segment
    client_count = (int *)shmat(shmid, NULL, 0);
    if (*client_count == -1) {
        perror("[-] Shared memory attachment failed\n");
        return 1;
    }

    // Initialize client count
    *client_count = 0;

    while (1) {
        // Accept incoming client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            printf("[-] Accepting connection failed\n");
            continue;
        }

        // Check if the server has reached maximum client capacity
        if (*client_count == MAX_CLIENTS) {
            printf("[-] Server capacity reached\n");
            printf("[*] Active clients: %d/%d\n", *client_count, MAX_CLIENTS);
            send(client_socket, "0", strlen("0"), 0);
            close(client_socket);
            continue;
        }

        // Notify the client that it's accepted
        send(client_socket, "1", strlen("1"), 0);
        (*client_count)++;

        printf("[+] New client connected - %s : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        printf("[*] Active clients: %d/%d\n", *client_count, MAX_CLIENTS);

        int client_id = ntohs(client_addr.sin_port);

        // Create a new process to handle the client communication
        if (fork() == 0) {
            close(server_socket); // Close the server socket in the child process
            handle_client(client_socket, client_id);
            close(client_socket);
            (*client_count)--;
            printf("[-] Client %d disconnected\n", client_id);
            printf("[*] Active clients: %d/%d\n", *client_count, MAX_CLIENTS);
            exit(0);
        }

        close(client_socket); // Close the client socket in the parent process
    }

    // Detach and remove shared memory
    shmdt(client_count);
    shmctl(shmid, IPC_RMID, NULL);

    close(server_socket);
    printf("[-] Server closed\n");
    return 0;
}
