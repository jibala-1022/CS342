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
            printf("A %d\n", client->socket_id);
            return client->socket_id;
        }
        client = client->next;
    }
    printf("NA -1\n");
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
    printf("Clients: ");
    while(client){
        printf("%s%d", client->name, client->socket_id);
        client = client->next;
    }
    printf("_\n");
}

void broadcast(struct ClientList* clientList, int client_socket, char* msg) {
    printf("%s\n", msg);
    // if (send(client_socket, "0", strlen("0"), 0) == -1) {
    //     perror("[-] Sending client failed\n");
    // }
    struct Client* client = clientList->head->next;
    while(client){
        if(client->socket_id != client_socket){
            if (send(client->socket_id, msg, strlen(msg), 0) == -1) {
                perror("[-] Sending receiver failed\n");
            }
        }
        client = client->next;
    }
}

void message(struct ClientList* clientList, int client_socket, char* msg, char* receiver_name) {
    printf("%s : %s\n",receiver_name, msg);
    int receiver_socket = getClientSocketID(clientList, receiver_name);
    if(receiver_socket == -1){
        // if (send(client_socket, "1", strlen("1"), 0) == -1) {
        //     perror("[-] Sending client failed\n");
        // }
        printf("Receiver does not exist\n");
    }
    else{
        // if (send(client_socket, "0", strlen("0"), 0) == -1) {
        //     perror("[-] Sending client failed\n");
        // }
        if (send(receiver_socket, msg, strlen(msg), 0) == -1) {
            perror("[-] Sending receiver failed\n");
        }
        printf("%d %d\n", client_socket, receiver_socket);
        printf("Sent to receiver\n");
    }
}

// Function to handle communication with a client
void* handleClient(void* args) {
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)args;
    struct ClientList* clientList = threadArgs->clientList;
    int* activeThreads = threadArgs->activeThreads;
    int threadIndex = threadArgs->threadIndex;
    int client_socket = threadArgs->client_socket;

    int bytes_received;
    int logged=0;
    char client_name[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    while (logged == 0) {
        // if (send(client_socket, "Enter username:\n", strlen("Enter username:\n"), 0) == -1) {
        //     perror("[-] Sending failed\n");
        //     break;
        // }

        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, client_name, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        printf("bytes rec %d\n", bytes_received);
        client_name[bytes_received-1] = '\0';

        if(getClientSocketID(clientList, client_name) == -1){
            if (send(client_socket, "00", strlen("00"), 0) == -1) {
                perror("[-] Sending failed\n");
                break;
            }
            addClient(clientList, client_name, client_socket);
            displayClients(clientList);
            printf("Client %s added\n", client_name);
            logged=1;
        }
        else{
            if (send(client_socket, "11", strlen("11"), 0) == -1) {
                perror("[-] Sending failed\n");
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

        printf("Client %s sent: %s\n", client_name, buffer);
        // fgets(buffer, BUFFER_SIZE, stdin);
        if (strncmp(buffer, "/private", strlen("/private")) == 0) {
            // Extract the receiver's name (the second word)
            char* token = strtok(buffer, " ");
            token = strtok(NULL, " "); // Get the second word
            if (token != NULL) {
                // Send the message to the receiver
                message(clientList, client_socket, buffer + strlen("/private") + strlen(token) + 2, token);
            }
        } else if (strncmp(buffer, "/broadcast", strlen("/broadcast")) == 0) {
            // Send a broadcast message (excluding the "/broadcast" prefix)
            broadcast(clientList, client_socket, buffer + strlen("/broadcast") + 1);
        } else {
            printf("Invalid command\n");
        }
    }

    activeThreads[threadIndex] = 0;
    deleteClient(clientList, client_name);
    printf("[-] Client %s disconnected\n", client_name);
    printf("[*] Active clients: %d/%d\n", clientList->size, clientList->capacity);
    close(client_socket);
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

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("[-] Socket creation failed\n");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("[-] Binding failed\n");
        return 1;
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("[-] Listening failed\n");
        return 1;
    }

    printf("[+] Server listening on port %d...\n", port);

    struct ClientList* clientList = initClientList(MAX_CLIENTS);
    struct ThreadArgs* threadArgs = (struct ThreadArgs*)malloc(sizeof(struct ThreadArgs));
    threadArgs->clientList = clientList;
    threadArgs->activeThreads = (int*)malloc(MAX_CLIENTS * sizeof(int));
    memset(threadArgs->activeThreads, 0, sizeof(threadArgs->activeThreads));

    displayClients(clientList);

    pthread_t threads[MAX_CLIENTS];

    while (1) {
        // Accept incoming client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            printf("[-] Accepting connection failed\n");
            continue;
        }

        for(int j=0; j<MAX_CLIENTS; j++){
            printf("%d ", threadArgs->activeThreads[j]);
        }
        printf("\n");

        threadArgs->client_socket = client_socket;
        int i;
        for(i=0; i<MAX_CLIENTS; i++){
            if(threadArgs->activeThreads[i]==0){
                printf("i=%d\n", i);
                threadArgs->activeThreads[i] = 1;
                threadArgs->threadIndex = i;
                if (send(client_socket, "0", strlen("0"), 0) == -1) {
                    perror("[-] Sending receiver failed\n");
                    break;
                }
                printf("[+] New client connected - %s : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                if(pthread_create(&threads[i], NULL, handleClient, (void*)threadArgs) != 0){
                    perror("pthread create failed");
                    break;
                }
                break;
            }
        }
        if(i == MAX_CLIENTS){
            if (send(client_socket, "1", strlen("1"), 0) == -1) {
                perror("[-] Sending receiver failed\n");
                break;
            }
            printf("[-] Server capacity reached\n");
            close(client_socket);
        }
    }

    for(int i=0; i<MAX_CLIENTS; i++){
        if(pthread_join(threads[i], NULL) != 0){
            perror("pthread join failed");
        }
    }

    close(server_socket);
    printf("[-] Server closed\n");
    return 0;
}
