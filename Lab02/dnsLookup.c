// $ gcc dnsLookup.c -o dns && ./dns
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

// Uncomment below line to execute in debug mode
// #define DEBUG

// Define the IP address and port of the DNS server
#define DNS_SERVER_IP_ADDR "8.8.8.8"
#define DNS_SERVER_PORT 53

// Define buffer sizes and constants
#define BUFFER_SIZE 200
#define DOMAIN_NAME_SIZE 50
#define CACHE_SIZE 5

// Define a structure for the DNS header
struct Header {
    uint16_t id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_rr_count;
    uint16_t authority_rr_count;
    uint16_t additional_rr_count;
};

// Define a structure for DNS question
struct Question {
    char* name;
    uint16_t type;
    uint16_t class;
};

// Define a structure for a node in the cache
struct Node {
    char* domain_name;
    char* ip_addr;
    char* response;
    struct Node* next;
    struct Node* prev;
};

// Define a structure for the cache
struct Cache {
    int capacity;
    int size;
    struct Node* head;
    struct Node* tail;
};

// Function to create a new node
struct Node* createNode(char* domain_name, char* ip_addr, char* response) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    newNode->domain_name = strdup(domain_name);
    newNode->ip_addr = strdup(ip_addr);
    newNode->response = strdup(response);
    newNode->next = NULL;
    newNode->prev = NULL;
    return newNode;
}

// Function to free a node
void freeNode(struct Node* node) {
    free(node->domain_name);
    free(node->ip_addr);
    free(node->response);
    free(node);
}

// Function to initialize the cache
struct Cache* initCache(int capacity) {
    struct Cache* cache = (struct Cache*)malloc(sizeof(struct Cache));
    cache->capacity = capacity;
    cache->size = 0;
    cache->head = createNode("", "", "");
    cache->tail = createNode("", "", "");
    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;
    return cache;
}

// Function to add a node to the front of the cache
void addToFront(struct Cache* cache, struct Node* node) {
    cache->head->next->prev = node;
    node->next = cache->head->next;
    cache->head->next = node;
    node->prev = cache->head;
    cache->size++;
}

// Function to move a node to the front of the cache
void moveToFront(struct Cache* cache, struct Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    cache->head->next->prev = node;
    node->next = cache->head->next;
    cache->head->next = node;
    node->prev = cache->head;
}

// Function to remove a node from the tail of the cache
void removeFromTail(struct Cache* cache) {
    struct Node* evicted_node = cache->tail->prev;
    cache->tail->prev->prev->next = cache->tail;
    cache->tail->prev = cache->tail->prev->prev;
    freeNode(evicted_node);
    cache->size--;
}

// Function to add a node to the cache
void addToCache(struct Cache* cache, char* domain_name, char* ip_addr, char* response) {
    if (cache->size == cache->capacity) {
        removeFromTail(cache);
    }
    struct Node* newNode = createNode(domain_name, ip_addr, response);
    addToFront(cache, newNode);
}

// Function to get an entry from the cache
char* getFromCache(struct Cache* cache, char* domain_name) {
    struct Node* node = cache->head->next;
    while (node != cache->tail) {
        if (strcmp(node->domain_name, domain_name) == 0) {
            moveToFront(cache, node);
            return node->ip_addr;
        }
        node = node->next;
    }
    return NULL;
}

// Function to free the cache and its nodes
void freeCache(struct Cache* cache) {
    struct Node* node = cache->head;
    while (node) {
        struct Node* temp = node;
        node = node->next;
        freeNode(temp);
    }
    free(cache);
}

// Function to create DNS flags
uint16_t createDnsFlags(int isResponse, int opcode, int isAuthoritative, int isTruncated, int recursionDesired, int recursionAvailable, int reservedBits, int responseCode) {
    uint16_t flags = 0;
    flags |= isResponse << 15;
    flags |= opcode << 11;
    flags |= isAuthoritative << 10;
    flags |= isTruncated << 9;
    flags |= recursionDesired << 8;
    flags |= recursionAvailable << 7;
    flags |= reservedBits << 4;
    flags |= responseCode;
    return flags;
}

// Function to convert a domain name to DNS format
void domainNametoDnsName(char* domain_name) {
    int i = strlen(domain_name);
    domain_name[i--] = '\0';
    int count = 0;
    while (i > 0) {
        if (domain_name[i - 1] == '.') {
            domain_name[i] = count;
            count = 0;
        } else {
            domain_name[i] = domain_name[i - 1];
            count++;
        }
        i--;
    }
    domain_name[0] = count;
}

// Function to generate a DNS query
size_t generateQuery(char* buffer, struct Header* dnsHeader, struct Question* dnsQuestion) {
    size_t name_sz = strlen(dnsQuestion->name) + 1;

    buffer[0] = dnsHeader->id >> 8;
    buffer[1] = dnsHeader->id;

    buffer[2] = dnsHeader->flags >> 8;
    buffer[3] = dnsHeader->flags;

    buffer[4] = dnsHeader->question_count >> 8;
    buffer[5] = dnsHeader->question_count;

    buffer[6] = dnsHeader->answer_rr_count >> 8;
    buffer[7] = dnsHeader->answer_rr_count;

    buffer[8] = dnsHeader->authority_rr_count >> 8;
    buffer[9] = dnsHeader->authority_rr_count;

    buffer[10] = dnsHeader->additional_rr_count >> 8;
    buffer[11] = dnsHeader->additional_rr_count;

    memcpy(buffer + 12, dnsQuestion->name, name_sz);

    buffer[12 + name_sz + 0] = dnsQuestion->type >> 8;
    buffer[12 + name_sz + 1] = dnsQuestion->type;

    buffer[12 + name_sz + 2] = dnsQuestion->class >> 8;
    buffer[12 + name_sz + 3] = dnsQuestion->class;

    return 12 + name_sz + 4;
}

// Main function
int main() {
    // Allocate memory for buffers and cache
    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    char* domain_name = (char*)malloc(DOMAIN_NAME_SIZE * sizeof(char));
    char* ip_addr;
    struct Cache* cache = initCache(CACHE_SIZE);

    // Create a UDP socket
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("[-] Socket creation failed\n");
        return 1;
    }
    #ifdef DEBUG
    printf("[+] Socket created successfully\n");
    #endif

    // Set up the DNS server address
    struct sockaddr_in dns_server_addr;
    memset(&dns_server_addr, 0, sizeof(dns_server_addr));
    dns_server_addr.sin_family = AF_INET;
    dns_server_addr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP_ADDR);
    dns_server_addr.sin_port = htons(DNS_SERVER_PORT);

    // Initialize DNS header and question
    struct Header dnsHeader;
    dnsHeader.id = getpid();
    dnsHeader.flags = createDnsFlags(0, 0, 0, 0, 1, 0, 0, 0);
    dnsHeader.question_count = 1;
    dnsHeader.answer_rr_count = 0;
    dnsHeader.authority_rr_count = 0;
    dnsHeader.additional_rr_count = 0;

    struct Question dnsQuestion;
    dnsQuestion.type = 1;
    dnsQuestion.class = 1;

    clock_t start_time, end_time;
    double time_taken;

    // Main loop to handle DNS queries
    while (1) {
        printf("[<] Enter the domain name to lookup (or '/exit' to quit): ");
        fgets(domain_name, DOMAIN_NAME_SIZE, stdin);

        if (strcmp(domain_name, "/exit\n") == 0) {
            break;
        }
        start_time = clock();

        // Convert domain name to DNS format and check cache
        domainNametoDnsName(domain_name);
        ip_addr = getFromCache(cache, domain_name);
        if (ip_addr) {
            end_time = clock();
            time_taken = ((double)(end_time - start_time) * 1000) / CLOCKS_PER_SEC;
            printf("[>] IP Address (Cached): %s  fetched in %.3f ms\n\n", ip_addr, time_taken);
            continue;
        }

        dnsQuestion.name = domain_name;

        // Generate and send DNS query
        size_t query_size = generateQuery(buffer, &dnsHeader, &dnsQuestion);
        if (sendto(client_socket, buffer, query_size, 0, (struct sockaddr*)&dns_server_addr, sizeof(dns_server_addr)) == -1) {
            perror("[-] Sending failed\n");
            break;
        }
        #ifdef DEBUG
        printf("[+] Sending successful\n");
        #endif

        // Receive DNS response
        socklen_t dns_server_addr_len = sizeof(dns_server_addr);
        memset(buffer, 0, BUFFER_SIZE);
        if (recvfrom(client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&dns_server_addr, &dns_server_addr_len) <= 0) {
            perror("[-] Receiving failed\n");
            break;
        }
        #ifdef DEBUG
        printf("[+] Receiving successful\n");
        #endif

        // Extract and display IP address from the response
        uint16_t res_answer_rr_count = buffer[6];
        res_answer_rr_count = (res_answer_rr_count << 8) | buffer[7];
        if (res_answer_rr_count == 0) {
            end_time = clock();
            time_taken = ((double)(end_time - start_time) * 1000) / CLOCKS_PER_SEC;
            printf("[>] IP Address: Not found\n\n");
            continue;
        }

        uint8_t ip[4];
        for (int i = 0; i < 4; i++) {
            ip[i] = buffer[query_size + 12 + i];
        }
        char buff[16];
        sprintf(buff, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        ip_addr = buff;
        end_time = clock();
        time_taken = ((double)(end_time - start_time) * 1000) / CLOCKS_PER_SEC;
        printf("[>] IP Address: %s  fetched in %.3f ms\n\n", ip_addr, time_taken);

        // Add the result to the cache
        addToCache(cache, domain_name, ip_addr, buffer);
    }

    // Close the socket and free allocated memory
    close(client_socket);
    #ifdef DEBUG
    printf("[-] Client disconnected\n");
    #endif

    free(buffer);
    free(domain_name);
    freeCache(cache);
    #ifdef DEBUG
    printf("[-] Freed heap memory\n");
    #endif

    return 0;
}
