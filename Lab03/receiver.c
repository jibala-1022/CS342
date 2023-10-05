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

struct Packet {
    int seq;
    char data[DATA_SIZE];
    int checksum;
} sndpkt, rcvpkt;

int client_socket;
static struct timespec start_time = {0}, end_time = {0};

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

int main() {
    int server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOOPBACK);
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Binding failed");
        return 1;
    }

    if (listen(server_socket, 1) == -1) {
        perror("Listening failed");
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    if((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len)) == -1){
        perror("Accepting connection failed");
        return 1;
    }
    printf("Client connected - %s : %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    int seq = 0;
    strcpy(sndpkt.data, "ACK");
    sndpkt.checksum = 0;
    rcvpkt.seq = seq;
    rcvpkt.checksum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    while (1) {
        rdt_rcv();

        if(!isCorrupted() && isAck(seq)){
            sndpkt.seq = seq;
        }
        else{
            sndpkt.seq = 1 - seq;
        }
        udt_send();
    }

    close(client_socket);
    printf("Client closed\n");
    close(server_socket);
    printf("Server closed\n");
    return 0;
}
