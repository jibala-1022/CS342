#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024
#define NAME_SIZE 30

struct Client {
    char* name;
    int socket_id;
    struct Client* next;
};

struct ClientList {
    int size;
    int capacity;
    struct Client* head;
};

struct ThreadArgs {
    struct ClientList* clientList;
    int* activeThreads;
    int threadIndex;
    int client_socket;
};

struct Client* createClient(char* name, int socket_id) {
    struct Client* client = (struct Client*)malloc(sizeof(struct Client));
    client->name = strdup(name);
    client->socket_id = socket_id;
    client->next = NULL;
    return client;
}

void freeClient(struct Client* client){
    free(client->name);
    free(client);
}

struct ClientList* initClientList(int capacity) {
    struct ClientList* clientList = (struct ClientList*)malloc(sizeof(struct ClientList));
    clientList->capacity = capacity;
    clientList->size = 0;
    clientList->head = createClient("", -1);
    return clientList;
}

int getClientSocketID(struct ClientList* clientList, char* name){
    struct Client* client = clientList->head->next;
    while(client){
        if(strcmp(name, client->name) == 0){
            // printf("A %d\n", client->socket_id);
            return client->socket_id;
        }
        client = client->next;
    }
    // printf("NA -1\n");
    return -1;
}

void addClient(struct ClientList* clientList, char* name, int socket_id){
    struct Client* client = createClient(name, socket_id);
    client->next = clientList->head->next;
    clientList->head->next = client;
    clientList->size++;
}

void deleteClient(struct ClientList* clientList, char* name){
    struct Client* client = clientList->head;
    while(client->next){
        if(strcmp(name, client->next->name) == 0){
            struct Client* evictClient = client->next;
            client->next = client->next->next;
            freeClient(evictClient);
            clientList->size--;
            break;
        }
        client = client->next;
    }
}

void displayClients(struct ClientList* clientList){
    struct Client* client = clientList->head->next;
    printf("Clients in the server:\n");
    int count=0;
    while(client){
        printf("%d. %s %d\n", ++count, client->name, client->socket_id);
        client = client->next;
    }
}

void freeClientList(struct ClientList* clientList) {
    struct Client* client = clientList->head;
    while (client) {
        struct Client* temp = client;
        client = client->next;
        freeClient(temp);
    }
}

void freeThreadArgs(struct ThreadArgs* threadArgs){
    free(threadArgs->activeThreads);
    free(threadArgs->clientList);
}

void broadcast(struct ClientList* clientList, char* client_name, int client_socket, char* msg) {
    printf("%s\n", msg);
    if (send(client_socket, "0", strlen("0"), 0) == -1) {
        printf("Sending client failed - broadcast\n");
    }
    char* named_msg = (char*)malloc(strlen("3Broadcast from ") + strlen(client_name) + strlen(": ") + strlen(msg) + 1);
    sprintf(named_msg, "3Broadcast from %s: %s", client_name, msg);
    struct Client* client = clientList->head->next;
    while(client){
        if(client->socket_id != client_socket){
            if (send(client->socket_id, named_msg, strlen(named_msg), 0) == -1) {
                printf("Sending receiver failed - broadcast\n");
            }
        }
        client = client->next;
    }
}

void message(struct ClientList* clientList, char* client_name, int client_socket, char* msg, char* receiver_name) {
    // printf("%s : %s\n",receiver_name, msg);
    int receiver_socket = getClientSocketID(clientList, receiver_name);
    if(receiver_socket == -1){
        if (send(client_socket, "2Invalid recipient", strlen("2Invalid recipient"), 0) == -1) {
            printf("Sending client 2 failed - message\n");
        }
        // printf("Receiver does not exist\n");
    }
    else{
        char* named_msg = (char*)malloc(strlen("3Private message from ") + strlen(client_name) + strlen(": ") + strlen(msg) + 1);
        sprintf(named_msg, "3Private message from %s: %s", client_name, msg);
        if (send(client_socket, "0", strlen("0"), 0) == -1) {
            printf("Sending client 1 failed - message\n");
        }
        if (send(receiver_socket, named_msg, strlen(named_msg), 0) == -1) {
            printf("Sending receiver failed - message\n");
        }
        // printf("%d %d\n", client_socket, receiver_socket);
        // printf("Sent to receiver\n");
    }
}

void* handleClient(void* args) {
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)args;
    struct ClientList* clientList = threadArgs->clientList;
    int* activeThreads = threadArgs->activeThreads;
    int threadIndex = threadArgs->threadIndex;
    int client_socket = threadArgs->client_socket;

    int bytes_received;
    int logged=0;
    char client_name[NAME_SIZE];
    char buffer[BUFFER_SIZE];

    while (logged == 0) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, client_name, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        // printf("bytes rec %d\n", bytes_received);
        client_name[bytes_received-1] = '\0';
        // printf("name%srec\n", client_name);

        if(getClientSocketID(clientList, client_name) == -1){
            if (send(client_socket, "0", strlen("0"), 0) == -1) {
                printf("Sending failed - handleClient logged\n");
                break;
            }
            addClient(clientList, client_name, client_socket);
            printf("%s joined the chat\n", client_name);
            displayClients(clientList);
            printf("Active clients: %d/%d\n", clientList->size, clientList->capacity);
            logged=1;
        }
        else{
            if (send(client_socket, "1", strlen("1"), 0) == -1) {
                printf("Sending failed - handleClient not logged\n");
                break;
            }
            printf("Username exists\n");
        }
    }

    while (logged == 1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received-1] = '\0';

        // printf("Client %s sent: %s\n", client_name, buffer);
        // if (strncmp(buffer, "/private", strlen("/private")) == 0) {
        //     // Extract the receiver's name (the second word)
        //     char* token = strtok(buffer, " ");
        //     token = strtok(NULL, " "); // Get the second word
        //     if (token != NULL) {
        //         // Send the message to the receiver
        //         message(clientList, client_name, client_socket, buffer + strlen("/private") + strlen(token) + 2, token);
        //     }
        // } 
        if (buffer[0] == '@') {
            // Find the first space character after the "@" symbol
            char* space_pos = strchr(buffer, ' ');
            if (space_pos != NULL) {
                // Extract the recipient's name
                int recipient_name_length = space_pos - buffer - 1; // subtract 1 to exclude the "@" symbol
                char recipient_name[recipient_name_length + 1]; // +1 for null-terminator
                strncpy(recipient_name, buffer + 1, recipient_name_length);
                recipient_name[recipient_name_length] = '\0';
                // Extract the message
                char* message_text = space_pos + 1;
                // Send the message to the recipient
                message(clientList, client_name, client_socket, message_text, recipient_name);
            }
        }
        else if (strncmp(buffer, "/broadcast", strlen("/broadcast")) == 0) {
            // Send a broadcast message (excluding the "/broadcast" prefix)
            broadcast(clientList, client_name, client_socket, buffer + strlen("/broadcast") + 1);
        } 
        else if(strlen(buffer) != 0) {
            if (send(client_socket, "1Invalid message format", strlen("1Invalid message format"), 0) == -1) {
                printf("Sending client failed - ack\n");
            }
            // printf("Invalid command\n");
        }
    }

    activeThreads[threadIndex] = 0;
    deleteClient(clientList, client_name);
    printf("%s left the chat\n", client_name);
    printf("Active clients: %d/%d\n", clientList->size, clientList->capacity);
    close(client_socket);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Socket creation failed\n");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("Socket binding failed\n");
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        printf("Socket listening failed\n");
        return 1;
    }

    printf("Server listening on port %d...\n", port);

    struct ClientList* clientList = initClientList(MAX_CLIENTS);

    struct ThreadArgs* threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    threadArgs->clientList = clientList;
    threadArgs->activeThreads = (int*)malloc(MAX_CLIENTS * sizeof(int));
    memset(threadArgs->activeThreads, 0, sizeof(threadArgs->activeThreads));

    pthread_t threads[MAX_CLIENTS];

    while (1) {
        // Accept incoming client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            printf("Accepting failed\n");
            continue;
        }

        // for(int j=0; j<MAX_CLIENTS; j++){
        //     printf("%d ", threadArgs->activeThreads[j]);
        // }
        // printf("\n");

        threadArgs->client_socket = client_socket;
        int i;
        for(i=0; i<MAX_CLIENTS; i++){
            if(threadArgs->activeThreads[i]==0){
                // printf("i=%d\n", i);
                threadArgs->activeThreads[i] = 1;
                threadArgs->threadIndex = i;
                if (send(client_socket, "0", strlen("0"), 0) == -1) {
                    printf("Sending receiver failed\n");
                    break;
                }
                printf("New client connected - %s : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                if(pthread_create(&threads[i], NULL, handleClient, (void*)threadArgs) != 0){
                    printf("pthread create failed");
                    break;
                }
                break;
            }
        }
        if(i == MAX_CLIENTS){
            if (send(client_socket, "1", strlen("1"), 0) == -1) {
                printf("Sending receiver failed\n");
                break;
            }
            printf("Server capacity reached\n");
            close(client_socket);
        }
    }

    for(int i=0; i<MAX_CLIENTS; i++){
        if(pthread_join(threads[i], NULL) != 0){
            printf("pthread join failed");
        }
    }

    close(server_socket);
    printf("Server closed\n");
    return 0;
}
