#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>
// #include <errno.h>

struct ThreadArgs {
    int client_socket;
    int* server_status;
};

#define BUFFER_SIZE 1024

void* receive(void* args){
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)args;
    int client_socket = threadArgs->client_socket;
    int* server_status = threadArgs->server_status;

    char buffer[BUFFER_SIZE];
    int bytes_received;
    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        printf("byrec %d\n", bytes_received);
        if (bytes_received <= 0) {
            perror("[-] Receiving failed\n");
            *server_status = 0;
            break;
        }
        buffer[bytes_received] = '\0';
        if(buffer[0] == '1'){
            printf("Invalid command\n");
        }
        else if(buffer[0] == '2'){
            printf("Invalid recipient\n");
        }
        else printf("%s\n", buffer);
    }
    printf("Server stopped\n");
    // close(client_socket);
}

// void* auth(void* args){
//     struct ThreadArgs* threadArgs = (struct ThreadArgs*)args;
//     int* server_status = threadArgs->server_status;
//     int client_socket = threadArgs->client_socket;
//     char buffer[BUFFER_SIZE];
//     int bytes_received;
//     while(1) {
//         memset(buffer, 0, BUFFER_SIZE);
//         bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
//         printf("byrec by auth %d\n", bytes_received);
//         if (bytes_received <= 0) {
//             perror("[-] Receiving failed\n");
//             *server_status = 0;
//             break;
//         }
//     }
//     printf("Server stopped at auth\n");
// }

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("[-] Usage: %s <server_ip_address> <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[2]);
    char* server_ip = argv[1];

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
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    if (buffer[0] == '1') {
        perror("[-] Server capacity reached\n");
        close(client_socket);
        return 1;
    }
    printf("[+] Connected to server...\n\n");

    int bytes_received;
    int logged = 0;
    int server_status = 1;

    
    // printf("Error code: %d\n", errno);

    struct ThreadArgs* threadArgs;
    // threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    // threadArgs->client_socket = client_socket;
    // threadArgs->server_status = &server_status;

    // pthread_t auth_thread;
    // if(pthread_create(&auth_thread, NULL, auth, (void*)threadArgs) != 0){
    //     perror("pthread create failed");
    //     return 1;
    // }

    while (logged == 0) {
        printf("Enter username: ");

        fgets(buffer, BUFFER_SIZE, stdin);
        printf("s%ds", buffer[0]);
        if(buffer[0] == '\n'){
            continue;
        }

        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("[-] Sending failed\n");
            break;
        }
        if (strcmp(buffer, "/exit\n") == 0) {
            printf("[-] Connection closed by client\n");
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            perror("[-] Receiving 2 failed\n");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        printf("%d", buffer[0]);
        printf("bytes rec %d\n", bytes_received);
        // if (buffer[0] == '0') {
        if (buffer[0] == '0') {
            printf("Connected\n");
            logged = 1;
            break;
        }
        else{
            printf("Username exists\n");
        }
    }

    threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    threadArgs->client_socket = client_socket;
    threadArgs->server_status = &server_status;

    pthread_t receive_thread;
    if(pthread_create(&receive_thread, NULL, receive, (void*)threadArgs) != 0){
        perror("pthread create failed");
        return 1;
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    printf("%d\n", flags);



    while(logged == 1){
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        if (server_status==0 || send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("[-] Sending failed\n");
            break;
        }
        if (strcmp(buffer, "/exit\n") == 0) {
            printf("[-] Connection closed by client\n");
            break;
        }
    }
    
    // Close the client socket
    close(client_socket);

    printf("[-] Client closed\n");

    return 0;
}
