#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define LOOPBACK "127.0.0.1"
#define PORT 8080
#define DATA_SIZE 4 * 5
#define DELAY_PROP_MIN 2
#define DELAY_PROP_MAX 5
#define P_LOST 0
#define P_CORRUPTED 0
#define TIMEOUT 11

struct Packet {
    int seq;
    char data[DATA_SIZE];
    int checksum;
} sndpkt, rcvpkt;

int client_socket;
static struct timespec start_time = {0}, end_time = {0};
struct itimerval timer;

void tick(){
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    int time_elapsed = end_time.tv_sec - start_time.tv_sec;
    printf(">> %2d: ", time_elapsed);
}

int isLost(){
    return rand() % 100 < P_LOST * 100;
}

int isCorrupted(){
    return rand() % 100 < P_CORRUPTED * 100;
}

int isAck(int seq){
    return rcvpkt.seq == seq;
}

void print_pkt(struct Packet* packet){
    printf("\tSEQ = %d\n", packet->seq);
    printf("\tDATA = %s\n", packet->data);
    printf("\tCHECKSUM = %d\n", packet->checksum);
}

void udt_send(){
    pid_t pid;

    tick();
    printf("Packet sent\n");
    print_pkt(&sndpkt);
    
    if((pid = fork()) == -1){
        perror("Process creation failed");
        return;
    }
    else if(pid == 0){
        int delay_seconds = DELAY_PROP_MIN + rand() % (DELAY_PROP_MAX - DELAY_PROP_MIN + 1);
        printf("Prop delay %ds\n", delay_seconds);
        sleep(delay_seconds);
        tick();

        if(isLost()){
            printf("Packet Lost\n");
            return;
        }
        if (send(client_socket, &sndpkt, sizeof(struct Packet), 0) == -1) {
            perror("Sending failed");
            return;
        }
        printf("Packet reached\n");
        exit(0);
    }
}

void rdt_rcv(){
    memset(&rcvpkt, 0, sizeof(struct Packet));
    if (recv(client_socket, &rcvpkt, sizeof(struct Packet), 0) == -1) {
        perror("Receiving failed");
        return;
    }
    tick();
    printf("Packet received\n");
    print_pkt(&rcvpkt);
}

void rdt_send(char* buffer, int seq){
    struct sigaction sa;
    sa.sa_handler = udt_send;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, NULL);

    timer.it_value.tv_sec = TIMEOUT;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = TIMEOUT;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return;
    }

    int checksum = 0;
    sndpkt.seq = seq;
    strcpy(sndpkt.data, buffer);
    sndpkt.checksum = checksum;
    
    while(1){
        udt_send();

        rdt_rcv();
        if(isCorrupted()){
            printf("Corrupted\n");
            continue;
        }
        if(isAck(seq)){
            printf("ACKed\n");
            break;
        }
    }
    strcpy(buffer, rcvpkt.data);
}

int main() {
    struct sockaddr_in server_addr;

    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOOPBACK);
    server_addr.sin_port = htons(PORT);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to server...\n");
        
    char buffer[DATA_SIZE];
    int seq = 0;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    // while (1) {
        printf("Enter a message: ");
        fgets(buffer, DATA_SIZE, stdin);
        buffer[strlen(buffer) - 1] = 0;

        if (strcmp(buffer, "/exit") == 0) {
            printf("Connection closed by client\n");
            // break;
        }

        rdt_send(buffer, seq);
        seq = 1 - seq;
    // }

    // Close the client socket
    close(client_socket);

    printf("Client closed\n");

    return 0;
}