// gcc -pthread q3_server.c -o server && ./server <server_port>
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

// Structure to represent a connected client
typedef struct Client {
    char* name;
    int socket_id;
    struct Client* next;
} Client;

// Structure to manage the list of connected clients
typedef struct ClientList {
    int size;
    int capacity;
    struct Client* head;
} ClientList;

// Structure to hold thread arguments
typedef struct ThreadArgs {
    struct ClientList* clientList;
    int* activeThreads;
    int threadIndex;
    int client_socket;
} ThreadArgs;

// Function to create a new client structure
Client* createClient(char* name, int socket_id) {
    Client* client = (Client*)malloc(sizeof(Client));
    client->name = strdup(name);
    client->socket_id = socket_id;
    client->next = NULL;
    return client;
}

// Function to free memory occupied by a client
void freeClient(Client* client) {
    free(client->name);
    free(client);
}

// Function to initialize the client list
ClientList* initClientList(int capacity) {
    ClientList* clientList = (ClientList*)malloc(sizeof(ClientList));
    clientList->capacity = capacity;
    clientList->size = 0;
    // Create a dummy head node for easier management
    clientList->head = createClient("", -1);
    return clientList;
}

// Function to get the socket ID of a client by their name
int getClientSocketID(ClientList* clientList, char* name) {
    Client* client = clientList->head->next;
    while (client) {
        if (strcmp(name, client->name) == 0) {
            return client->socket_id;
        }
        client = client->next;
    }
    return -1;
}

// Function to add a client to the list
void addClient(ClientList* clientList, char* name, int socket_id) {
    Client* client = createClient(name, socket_id);
    client->next = clientList->head->next;
    clientList->head->next = client;
    clientList->size++;
}

// Function to delete a client from the list
void deleteClient(ClientList* clientList, char* name) {
    Client* client = clientList->head;
    while (client->next) {
        if (strcmp(name, client->next->name) == 0) {
            Client* evictClient = client->next;
            client->next = client->next->next;
            freeClient(evictClient);
            clientList->size--;
            break;
        }
        client = client->next;
    }
}

// Function to display the list of connected clients
void displayClients(ClientList* clientList) {
    Client* client = clientList->head->next;
    printf("Clients in the server:\n");
    int count = 0;
    while (client) {
        printf("%d. %s %d\n", ++count, client->name, client->socket_id);
        client = client->next;
    }
}

// Function to free memory occupied by the client list
void freeClientList(ClientList* clientList) {
    Client* client = clientList->head;
    while (client) {
        Client* temp = client;
        client = client->next;
        freeClient(temp);
    }
}

// Function to free memory occupied by thread arguments
void freeThreadArgs(ThreadArgs* threadArgs) {
    free(threadArgs->activeThreads);
    free(threadArgs->clientList);
}

// Function to broadcast a message to all clients except the sender
void broadcast(ClientList* clientList, char* client_name, int client_socket, char* msg) {
    // Acknowledge the sender
    if (send(client_socket, "0", strlen("0"), 0) == -1) {
        printf("Sending client failed - broadcast\n");
    }
    // Prepare the message with sender's name
    char* named_msg = (char*)malloc(strlen("3Broadcast from ") + strlen(client_name) + strlen(": ") + strlen(msg) + 1);
    sprintf(named_msg, "3Broadcast from %s: %s", client_name, msg);
    // Send the message to all clients except the sender
    Client* client = clientList->head->next;
    while (client) {
        if (client->socket_id != client_socket) {
            if (send(client->socket_id, named_msg, strlen(named_msg), 0) == -1) {
                printf("Sending receiver failed - broadcast\n");
            }
        }
        client = client->next;
    }
    free(named_msg);
}

// Function to send a private message to a specific client
void message(ClientList* clientList, char* client_name, int client_socket, char* msg, char* receiver_name) {
    // Get the socket ID of the recipient
    int receiver_socket = getClientSocketID(clientList, receiver_name);
    if (receiver_socket == -1) {
        // If the recipient does not exist, send an error message to the sender
        if (send(client_socket, "2Invalid recipient", strlen("2Invalid recipient"), 0) == -1) {
            printf("Sending client 2 failed - message\n");
        }
    } else {
        // Prepare the message with sender's name
        char* named_msg = (char*)malloc(strlen("3Private message from ") + strlen(client_name) + strlen(": ") + strlen(msg) + 1);
        sprintf(named_msg, "3Private message from %s: %s", client_name, msg);
        // Acknowledge the sender
        if (send(client_socket, "0", strlen("0"), 0) == -1) {
            printf("Sending client 1 failed - message\n");
        }
        // Send the message to the recipient
        if (send(receiver_socket, named_msg, strlen(named_msg), 0) == -1) {
            printf("Sending receiver failed - message\n");
        }
        free(named_msg);
    }
}

// Function to handle communication with a client in a thread
void* handleClient(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    ClientList* clientList = threadArgs->clientList;
    int* activeThreads = threadArgs->activeThreads;
    int threadIndex = threadArgs->threadIndex;
    int client_socket = threadArgs->client_socket;

    int bytes_received;
    int logged = 0;
    char client_name[NAME_SIZE];
    char buffer[BUFFER_SIZE];

    // Handle the client's login process
    while (logged == 0) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, client_name, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        client_name[bytes_received - 1] = '\0';

        if (getClientSocketID(clientList, client_name) == -1) {
            // Acknowledge the client and add them to the client list
            if (send(client_socket, "0", strlen("0"), 0) == -1) {
                printf("Sending failed - handleClient logged\n");
                break;
            }
            addClient(clientList, client_name, client_socket);
            printf("%s joined the chat\n", client_name);
            displayClients(clientList);
            printf("Active clients: %d/%d\n", clientList->size, clientList->capacity);
            logged = 1;
        } else {
            // Inform the client that the username already exists
            if (send(client_socket, "1", strlen("1"), 0) == -1) {
                printf("Sending failed - handleClient not logged\n");
                break;
            }
            printf("Username exists\n");
        }
    }

    // Handle chat messages from the client
    while (logged == 1) {
        memset(buffer, 0, BUFFER_SIZE);
        bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received - 1] = '\0';

        if (buffer[0] == '@') {
            // Private message format: "@recipient message"
            char* space_pos = strchr(buffer, ' ');
            if (space_pos != NULL) {
                int recipient_name_length = space_pos - buffer - 1;
                char recipient_name[recipient_name_length + 1];
                strncpy(recipient_name, buffer + 1, recipient_name_length);
                recipient_name[recipient_name_length] = '\0';
                char* message_text = space_pos + 1;
                message(clientList, client_name, client_socket, message_text, recipient_name);
            }
        } else if (strncmp(buffer, "/broadcast", strlen("/broadcast")) == 0) {
            // Broadcast message format: "/broadcast message"
            broadcast(clientList, client_name, client_socket, buffer + strlen("/broadcast") + 1);
        } else if (strlen(buffer) != 0) {
            // Invalid message format (non-empty message without a command)
            if (send(client_socket, "1Invalid message format", strlen("1Invalid message format"), 0) == -1) {
                printf("Sending client failed - ack\n");
            }
        }
    }

    // Mark the thread as inactive and remove the client from the list
    activeThreads[threadIndex] = 0;
    deleteClient(clientList, client_name);
    printf("%s left the chat\n", client_name);
    printf("Active clients: %d/%d\n", clientList->size, clientList->capacity);
    close(client_socket);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create a server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        printf("Socket creation failed\n");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the server socket to the specified port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        printf("Socket binding failed\n");
        return 1;
    }

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        printf("Socket listening failed\n");
        return 1;
    }

    printf("Server listening on port %d...\n", port);

    // Initialize the client list and thread arguments
    ClientList* clientList = initClientList(MAX_CLIENTS);

    ThreadArgs* threadArgs = (ThreadArgs*)malloc(sizeof(ThreadArgs));
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

        // Set up thread arguments for the new client
        threadArgs->client_socket = client_socket;
        int i;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (threadArgs->activeThreads[i] == 0) {
                threadArgs->activeThreads[i] = 1;
                threadArgs->threadIndex = i;
                // Acknowledge the client's connection
                if (send(client_socket, "0", strlen("0"), 0) == -1) {
                    printf("Sending receiver failed\n");
                    break;
                }
                printf("New client connected - %s : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                // Create a new thread to handle the client
                if (pthread_create(&threads[i], NULL, handleClient, (void*)threadArgs) != 0) {
                    printf("pthread create failed");
                    break;
                }
                break;
            }
        }
        if (i == MAX_CLIENTS) {
            // If the server is at capacity, inform the client and close the connection
            if (send(client_socket, "1", strlen("1"), 0) == -1) {
                printf("Sending receiver failed\n");
                break;
            }
            printf("Server capacity reached\n");
            close(client_socket);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            printf("pthread join failed");
        }
    }

    // Close the server socket and free allocated memory
    close(server_socket);
    freeClientList(clientList);
    freeThreadArgs(threadArgs);

    printf("Server closed\n");
    return 0;
}
