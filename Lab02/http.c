#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stddef.h>

// Define data structures
struct WebPage {
    char url[255];
    char content[16384]; // Adjust size as needed
};

// Linked list node to track page accesses
struct CacheNode {
    struct WebPage page;
    struct CacheNode* next;
};

// Cache configuration
#define MAX_CACHE_SIZE 5

// Global variables
struct CacheNode* cache = NULL;
int cacheSize = 0;

// Function prototypes
void initializeCache();
struct CacheNode* findPage(const char* url);
void moveToFront(struct CacheNode* node);
void evictLRU();
char* retrieveWebPage(const char* url);
void fetchWebPage(const char* url);
void displayCacheContents();
int sendHttpRequest(const char* hostname, const char* path, char* response);

// Initialize the cache
void initializeCache() {
    cache = NULL;
    cacheSize = 0;
}

// Find a page in the cache
struct CacheNode* findPage(const char* url) {
    struct CacheNode* current = cache;
    while (current != NULL) {
        if (strcmp(current->page.url, url) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// Move a page to the front (most recently used)
void moveToFront(struct CacheNode* node) {
    if (node == cache) {
        return; // Already at the front
    }

    // Remove the node from its current position
    struct CacheNode* prev = cache;
    while (prev->next != node) {
        prev = prev->next;
    }
    prev->next = node->next;

    // Move the node to the front
    node->next = cache;
    cache = node;
}

// Evict the least recently used page
void evictLRU() {
    if (cache == NULL) {
        return; // Cache is empty
    }

    if (cacheSize == 1) {
        free(cache);
        cache = NULL;
    } else {
        struct CacheNode* prev = cache;
        struct CacheNode* current = cache->next;
        while (current->next != NULL) {
            prev = current;
            current = current->next;
        }
        free(current);
        prev->next = NULL;
    }

    cacheSize--;
}





// Send an HTTP GET request and receive the response
int sendHttpRequest(const char* hostname, const char* path, char* response) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent* server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        return -1;
    }

    server = gethostbyname(hostname);
    if (server == NULL) {
        perror("Error getting host by name");
        return -1;
    }

    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&server_addr.sin_addr.s_addr, server->h_length);
    server_addr.sin_port = htons(80);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        return -1;
    }

    // Build the HTTP GET request with headers
    char request[1024];
    sprintf(request, "GET %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "User-Agent: SimpleHTTPClient\r\n"
                     "\r\n",
            path, hostname);

    // Send the request
    if (write(sockfd, request, strlen(request)) < 0) {
        perror("Error writing to socket");
        return -1;
    }

    // Receive and store the response
    int totalBytes = 0;
    int bytesRead;
    char buffer[4096]; // Temporary buffer to store response chunks

    while ((bytesRead = read(sockfd, buffer, sizeof(buffer))) > 0) {
        // Append the received chunk to the response
        memcpy(response + totalBytes, buffer, bytesRead);
        totalBytes += bytesRead;
    }

    close(sockfd);
    return totalBytes;
}









// Retrieve a web page from the cache or fetch it via HTTP GET
char* retrieveWebPage(const char* url) {
    struct CacheNode* node = findPage(url);

    if (node != NULL) {
        moveToFront(node);
        return node->page.content;
    } else {
        if (cacheSize >= MAX_CACHE_SIZE) {
            evictLRU();
        }
        fetchWebPage(url);
        return retrieveWebPage(url); // Retry after fetching
    }
}







// Fetch a web page via HTTP GET
void fetchWebPage(const char* url) {


// Extract hostname and path from the URL
char hostname[255];
char path[255];
if (sscanf(url, "http://%[^/]/%s", hostname, path) != 2) {
    fprintf(stderr, "Invalid URL format: %s\n", url);
    return;
}


    // Check if the URL starts with "http://"
    if (strncmp(url, "http://", 7) != 0) {
        fprintf(stderr, "Invalid URL format: %s\n", url);
        return;
    }

    // Find the first '/' character after "http://"
    const char* pathStart = strchr(url + 7, '/');
    if (pathStart == NULL) {
        fprintf(stderr, "Invalid URL format: %s\n", url);
        return;
    }

    // Calculate the position of the hostname and path
    ptrdiff_t hostnameLength = pathStart - (url + 7);
    
    
    
    //hostname = url + 7;
    //path = pathStart;
    
    // You cannot assign directly to arrays, so use strncpy to copy the values
	strncpy(hostname, url + 7, hostnameLength);
	hostname[hostnameLength] = '\0'; // Null-terminate the string

	strncpy(path, pathStart, sizeof(path) - 1);
	path[sizeof(path) - 1] = '\0'; // Null-terminate the string
    
    
    

    // Send an HTTP GET request and receive the response
    char response[16384];
    int bytesReceived = sendHttpRequest(hostname, path, response);

    // Create a new cache node
    struct WebPage newPage;
    strcpy(newPage.url, url);
    if (bytesReceived > 0) {
        strncpy(newPage.content, response, sizeof(newPage.content) - 1);
    } else {
        strcpy(newPage.content, "Error fetching page.");
    }

    struct CacheNode* newNode = malloc(sizeof(struct CacheNode));
    newNode->page = newPage;
    newNode->next = cache;

    cache = newNode;
    cacheSize++;
}








// Display cache contents
void displayCacheContents() {
    struct CacheNode* current = cache;
    printf("Cache Contents (Most to Least Recently Used):\n");
    while (current != NULL) {
        printf("%s\n", current->page.url);
        current = current->next;
    }
}

int main() {
    char url[255];

    initializeCache();

    while (1) {
    printf("Enter URL (or 'exit' to quit): ");
    fgets(url, sizeof(url), stdin);
    
    printf("Entered URL: %s", url);

    // Remove trailing newline
    if (url[strlen(url) - 1] == '\n') {
        url[strlen(url) - 1] = '\0';
    }

    if (strcmp(url, "exit") == 0) {
        break; // Exit the loop and quit the program
    }

    char* content = retrieveWebPage(url);
    printf("Page content:\n%s\n", content);
    displayCacheContents();
}


    return 0;
}

