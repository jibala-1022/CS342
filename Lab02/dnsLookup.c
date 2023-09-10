// gcc dnsLookup.c -o dnsLookup && ./dnsLookup
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <limits.h>

// DNS server IP address and port
#define DNS_SERVER_IP_ADDR "8.8.8.8"
#define DNS_SERVER_PORT 53

// Maximum buffer size, maximum domain name size, and cache size
#define BUFFER_SIZE 500
#define DOMAIN_NAME_SIZE 50
#define CACHE_SIZE 5

// Function to print debugging messages if DEBUG is defined
void debug(char* message){
    #ifdef DEBUG
    printf("%s", message);
    #endif
}

// Struct to represent the DNS header
typedef struct Header {
    uint16_t transaction_id;
    uint16_t flags;
    uint16_t question_count;
    uint16_t answer_rr_count;
    uint16_t authority_rr_count;
    uint16_t additional_rr_count;
} Header;

// Struct to represent a DNS question
typedef struct Question {
    char* name;
    uint16_t type;
    uint16_t class;
} Question;

// Struct to represent a node in the cache
typedef struct Node {
    char* domain_name;
    uint32_t ip_addr[20];
    time_t expiry_time;
    char* response;
    struct Node* next;
    struct Node* prev;
} Node;

// Struct to represent the cache
typedef struct Cache {
    int capacity;
    int size;
    struct Node* head;
    struct Node* tail;
} Cache;

// Function to parse the DNS response and extract IP addresses
void parseResponse(Node* node, char* response, uint16_t answer_rr_count){
    uint32_t* ip_addr = node->ip_addr;
    uint32_t min_ttl = UINT_MAX;
    time_t timestamp = time(NULL);

    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t data_length;
    uint32_t ip;

    int count=0;
    for(int i=0; i<answer_rr_count; i++){
        response += 2;

        // Extract DNS record information
        type = response[0];
        type = (type << 8) + response[1];
        response += 2;

        class = response[0];
        class = (class << 8) + response[1];
        response += 2;

        ttl = response[0];
        ttl = (ttl << 8) + response[1];
        ttl = (ttl << 8) + response[2];
        ttl = (ttl << 8) + response[3];
        response += 4;

        data_length = response[0];
        data_length = (data_length << 8) + response[1];
        response += 2;

        if(type == 1 && class == 1) { // If it's an IPv4 address (A record) and class is IN (Internet)
            // Extract and store the IP address
            ip = (uint8_t)response[0];
            ip = (ip << 8) | (uint8_t)response[1];
            ip = (ip << 8) | (uint8_t)response[2];
            ip = (ip << 8) | (uint8_t)response[3];
            response += 4;

            ip_addr[count] = ip;
            count++;
            min_ttl = (min_ttl < ttl) ? min_ttl : ttl; // Update minimum TTL
        }
        else {
            response += data_length; // Skip other record types
        }
    }
    ip_addr[count] = 0; // Terminate the IP address list
    node->expiry_time = timestamp + min_ttl; // Set the expiry time
}

// Function to create a new node for the cache
Node* createNode(char* domain_name, char* response, size_t query_size, uint16_t answer_rr_count) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->domain_name = strdup(domain_name);
    parseResponse(newNode, response + query_size, answer_rr_count);
    newNode->response = strdup(response);
    newNode->next = NULL;
    newNode->prev = NULL;
    return newNode;
}

// Function to free memory allocated for a node
void freeNode(Node* node) {
    free(node->domain_name);
    free(node->response);
    free(node);
}

// Function to add node to cache
void addNode(Cache* cache, Node* node) {
    cache->head->next->prev = node;
    node->next = cache->head->next;
    cache->head->next = node;
    node->prev = cache->head;
    cache->size++;
}

// Function to update existing to recently used to cache
void updateNode(Cache* cache, Node* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    cache->head->next->prev = node;
    node->next = cache->head->next;
    cache->head->next = node;
    node->prev = cache->head;
}

// Function to remove node from cache
void removeNode(Cache* cache, Node* node) {
    Node* evicted_node = cache->tail->prev;
    node->prev->next = node->next;
    node->next->prev = node->prev;
    freeNode(evicted_node);
    cache->size--;
}

// Function to display available authoritative IP addresses
void displayIp(Node* node) {
    uint32_t* ip_addr = node->ip_addr;
    while(*ip_addr != 0){
        printf(">> %d.%d.%d.%d\n", (uint8_t)((*ip_addr)>>24), (uint8_t)((*ip_addr)>>16), (uint8_t)((*ip_addr)>>8), (uint8_t)(*ip_addr));
        ip_addr++;
    }
}

// Function to initialize cache structure
Cache* initCache(int capacity) {
    Cache* cache = (Cache*)malloc(sizeof(Cache));
    cache->capacity = capacity;
    cache->size = 0;
    cache->head = createNode("", "", 0, 0);
    cache->tail = createNode("", "", 0, 0);
    cache->head->next = cache->tail;
    cache->tail->prev = cache->head;
    return cache;
}

// Function to query cache for a specific domain name, updating its position if found and not expired
Node* getFromCache(Cache* cache, char* domain_name) {
    Node* node = cache->head->next;
    time_t timestamp = time(NULL);
    while (node != cache->tail) {
        if (strcmp(node->domain_name, domain_name) == 0) {
            if(timestamp > node->expiry_time){
                removeNode(cache, node);
                printf("Cache expired\n");
                return NULL;
            }
            else{
                updateNode(cache, node);
                return node;
            }
        }
        node = node->next;
    }
    return NULL;
}

// Function to add new cache block
Node* addToCache(Cache* cache, char* domain_name, char* response, size_t query_size, uint16_t answer_rr_count) {
    if (cache->size == cache->capacity) {
        removeNode(cache, cache->tail->prev);
    }
    Node* newNode = createNode(domain_name, response, query_size, answer_rr_count);
    addNode(cache, newNode);
    return newNode;
}

// Function to deallocate cache structure
void freeCache(Cache* cache) {
    Node* node = cache->head;
    while (node) {
        Node* temp = node;
        node = node->next;
        freeNode(temp);
    }
}

// Function to create 16bit flag for DNS header
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

// Function to convert human readable domain name to dns name
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

// Function to generate DNS query from given parameters
size_t generateQuery(char* buffer, Header* dnsHeader, Question* dnsQuestion) {
    size_t name_sz = strlen(dnsQuestion->name) + 1;

    buffer[0] = dnsHeader->transaction_id >> 8;
    buffer[1] = dnsHeader->transaction_id;

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
    // Allocate memory
    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    char* domain_name = (char*)malloc(DOMAIN_NAME_SIZE * sizeof(char));
    char* ip_addr;
    Cache* cache = initCache(CACHE_SIZE);
    Node* cacheNode;

    // Client UDP socket setup
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed\n");
        return 1;
    }
    debug("Socket created successfully\n");

    struct sockaddr_in dns_server_addr;
    memset(&dns_server_addr, 0, sizeof(dns_server_addr));
    dns_server_addr.sin_family = AF_INET;
    dns_server_addr.sin_addr.s_addr = inet_addr(DNS_SERVER_IP_ADDR);
    dns_server_addr.sin_port = htons(DNS_SERVER_PORT);

    // Initialize DNS fields
    Header dnsHeader;
    dnsHeader.transaction_id = getpid();
    dnsHeader.flags = createDnsFlags(0, 0, 0, 0, 1, 0, 0, 0);
    dnsHeader.question_count = 1;
    dnsHeader.answer_rr_count = 0;
    dnsHeader.authority_rr_count = 0;
    dnsHeader.additional_rr_count = 0;

    Question dnsQuestion;
    dnsQuestion.type = 1;
    dnsQuestion.class = 1;

    clock_t start_time, end_time;
    double time_taken;

    while (1) {
        // Prompt for user input
        printf("Enter the domain name to lookup (or '/exit' to quit): ");
        fgets(domain_name, DOMAIN_NAME_SIZE, stdin);

        // Check if user wants to exit
        if (strcmp(domain_name, "/exit\n") == 0) {
            break; // Exit the loop
        }
        start_time = clock();

        // Convert domain name to DNS format and check cache
        domainNametoDnsName(domain_name);
        cacheNode = getFromCache(cache, domain_name);

        // If cached, display and continue
        if (cacheNode) {
            end_time = clock();
            time_taken = ((double)(end_time - start_time) * 1000) / CLOCKS_PER_SEC;
            printf("IP Address (Cached):\n");
            displayIp(cacheNode);
            printf("fetched in %.3f ms\n\n", time_taken);
            continue;
        }

        // Prepare DNS query and send it to the server
        dnsQuestion.name = domain_name;
        size_t query_size = generateQuery(buffer, &dnsHeader, &dnsQuestion);

        if (sendto(client_socket, buffer, query_size, 0, (struct sockaddr*)&dns_server_addr, sizeof(dns_server_addr)) == -1) {
            perror("Sending failed\n");
            break;
        }
        debug("Sending successful\n");

        // Receive and process the DNS server's response
        socklen_t dns_server_addr_len = sizeof(dns_server_addr);
        memset(buffer, 0, BUFFER_SIZE);

        if (recvfrom(client_socket, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&dns_server_addr, &dns_server_addr_len) <= 0) {
            perror("Receiving failed\n");
            break;
        }
        debug("Receiving successful\n");

        // Extract the answer count from the response
        uint16_t res_answer_rr_count = buffer[6];
        res_answer_rr_count = (res_answer_rr_count << 8) | buffer[7];

        // If no IP address found, display and continue
        if (res_answer_rr_count == 0) {
            end_time = clock();
            time_taken = ((double)(end_time - start_time) * 1000) / CLOCKS_PER_SEC;
            printf("IP Address: Not found\n\n");
            continue;
        }

        // Display the IP address, add to cache, and calculate time taken
        printf("IP Address:\n");
        cacheNode = addToCache(cache, domain_name, buffer, query_size, res_answer_rr_count);
        displayIp(cacheNode);
        end_time = clock();
        time_taken = ((double)(end_time - start_time) * 1000) / CLOCKS_PER_SEC;
        printf("fetched in %.3f ms\n\n", time_taken);
    }

    // Close the client
    close(client_socket);
    debug("Client disconnected\n");

    // Deallocate memory
    free(buffer);
    free(domain_name);
    freeCache(cache);
    debug("Freed heap memory\n");

    return 0;
}
