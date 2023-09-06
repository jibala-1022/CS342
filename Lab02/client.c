#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/select.h>

#define BUFFER_SIZE 1024

struct ThreadArgs {
    int client_socket;
    int* server_status;
};


void* receive(void* args){
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)args;
    int client_socket = threadArgs->client_socket;
    int* server_status = threadArgs->server_status;

    char buffer[BUFFER_SIZE];
    int bytes_received;

    while(1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        // printf("byrec %d\n", bytes_received);
        if (bytes_received <= 0) {
            printf("Receiving failed - receive\n");
            *server_status = 0;
            break;
        }
        buffer[bytes_received] = '\0';
        if(buffer[0] == '0'){
            printf("Sent successfully\n");
        }
        else if(buffer[0] == '1'){
            printf("Invalid message format\n");
        }
        else if(buffer[0] == '2'){
            printf("Invalid recipient\n");
        }
        else if(buffer[0] == '3'){
            printf("%s\n", buffer + 1);
        }
        else{
            printf("Garbage message\n");
        }
    }
    printf("Server terminated\n");
}


int main(int argc, char *argv[]) {
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
        printf("fgets 1\n");
        if(buffer[0] == '\n'){
            continue;
        }

        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            printf("Sending failed - auth\n");
            return 1;
        }
        if (strcmp(buffer, "/exit\n") == 0) {
            printf("Connection closed by client\n");
            return 1;
        }

        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Receiving failed - auth\n");
            return 1;
        }
        buffer[bytes_received] = '\0';

        if (buffer[0] == '0') {
            printf("Joined the chat\n");
            logged = 1;
            break;
        }
        else{
            printf("Username already exists\n");
        }
    }

    struct ThreadArgs* threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    threadArgs->client_socket = client_socket;
    threadArgs->server_status = &server_status;

    pthread_t receive_thread;
    if(pthread_create(&receive_thread, NULL, receive, (void*)threadArgs) != 0){
        printf("pthread create failed\n");
        return 1;
    }

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
    printf("%d\n", flags);


    while(logged == 1){
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);
        // printf("fgets 2\n");
        if (server_status==1 && send(client_socket, buffer, strlen(buffer), 0) == -1) {
            printf("Sending failed - chat\n");
            break;
        }
        if (strcmp(buffer, "/exit\n") == 0) {
            break;
        }
    }

    if(pthread_join(receive_thread, NULL) != 0){
            printf("pthread join failed\n");
        }
    
    // Close the client socket
    close(client_socket);
    free(threadArgs);

    printf("Client closed\n");

    return 0;
}
