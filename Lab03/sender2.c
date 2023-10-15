#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define LOOPBACK "127.0.0.1"
#define PORT 8080
#define DATA_SIZE 20
#define DELAY_PROP_MIN 2
#define DELAY_PROP_MAX 3
#define P_CORRUPTED 0.5

#define RED 91
#define GREEN 92
#define YELLOW 93

struct Packet {
    uint16_t seq;
    char data[DATA_SIZE];
    uint16_t checksum;
} sndpkt, rcvpkt;

int client_socket;
struct sockaddr_in server_addr;
socklen_t server_addr_len;
struct timespec start_time = {0}, end_time = {0};
FILE* fin;

void tick(){
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    int time_elapsed = end_time.tv_sec - start_time.tv_sec;
    printf("\033[1;100m %3d \033[0m\n", time_elapsed);
}

void print_pkt(struct Packet* packet, int color){
    printf("    \033[%dm SEQ = %d \033[0m\n", color, packet->seq);
    printf("    \033[%dm DATA = %s \033[0m\n", color, packet->data);
    printf("    \033[%dm CHECKSUM = %d \033[0m\n", color, packet->checksum);
}


uint16_t calculateChecksum(){
    uint32_t checksum = 0;
    checksum += sndpkt.seq;
    for(int i=0; i < DATA_SIZE; i+=2){
        checksum += ((uint16_t)sndpkt.data[i] << 8 | (uint16_t)sndpkt.data[i+1]); 
    }
    while(checksum > __UINT16_MAX__){
        checksum = ((checksum & 0xffff0000) >> 8) + (checksum & 0x0000ffff);
    }
    return ~checksum;
}

void corrupt(){
    double random = (double)rand() / RAND_MAX;
    if(random < 0.3){
        sndpkt.seq ^= 1;
    }
    else if(random < 0.6){
        int corruptIndex = (double)rand() / RAND_MAX * strlen(sndpkt.data);
        if(corruptIndex == strlen(sndpkt.data)) corruptIndex = 0;
        sndpkt.data[corruptIndex] ^= 1;
    }
    else{
        sndpkt.checksum ^= 1;
    }
}

int isCorrupted(){
    uint32_t checksum = 0;
    checksum += rcvpkt.seq;
    for(int i=0; i < DATA_SIZE; i+=2){
        checksum += ((uint16_t)rcvpkt.data[i] << 8 | (uint16_t)rcvpkt.data[i+1]); 
    }
    checksum += rcvpkt.checksum;
    while(checksum > 0x0000ffff){
        checksum = ((checksum & 0xffff0000) >> 8) + (checksum & 0x0000ffff);
    }
    return ((uint16_t)checksum ^ 0xffff);
}

int isAck(uint16_t seq){ return rcvpkt.seq == seq; }


void udt_send(){
    tick();
    printf("<< Packet sent\n");
    print_pkt(&sndpkt, YELLOW);

    if((double)rand() / RAND_MAX < P_CORRUPTED){
        corrupt();
    }

    int delay_seconds = DELAY_PROP_MIN + (double)rand() / RAND_MAX * (DELAY_PROP_MAX - DELAY_PROP_MIN + 1);
    sleep(delay_seconds);
    
    if (send(client_socket, &sndpkt, sizeof(struct Packet), 0) == -1) {
        perror("Sending failed");
        exit(1);
    }
}

void rdt_rcv(){
    memset(&rcvpkt, 0, sizeof(struct Packet));
    int bytes_received;
    while(1){
        if((bytes_received = recv(client_socket, &rcvpkt, sizeof(struct Packet), 0)) == -1) {
            perror("Receiving failed");
            exit(1);
        }
        break;
    }
    tick();
    printf(">> Packet received - ");
}

void make_pkt(char* buffer, int seq){
    sndpkt.seq = seq;
    strcpy(sndpkt.data, buffer);
    sndpkt.checksum = calculateChecksum();
}

void rdt_send(char* buffer, int seq){
    while(1){
        make_pkt(buffer, seq);
        udt_send();
        rdt_rcv();

        if(isCorrupted()){
            printf("Corrupted\n");
            print_pkt(&rcvpkt, RED);
        }
        else if(!isAck(seq)){
            printf("Old ACK\n");
            print_pkt(&rcvpkt, RED);
        }
        else{
            printf("ACKed\n");
            print_pkt(&rcvpkt, GREEN);
            break;
        }
    }
}

int main() {
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOOPBACK);
    server_addr.sin_port = htons(PORT);
    server_addr_len = sizeof(server_addr);

    if((fin = fopen("sender.txt", "r")) == NULL){
        perror("File opening failed");
        exit(1);
    }

    if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_len) == -1) {
        perror("[-] Connection failed\n");
        return 1;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    printf("Connected to server...\n");


    char buffer[DATA_SIZE];
    memset(buffer, 0, DATA_SIZE);
    uint16_t seq = 0;

    int itemsRead;
    while ((itemsRead = fscanf(fin, "%s", buffer)) == 1) {
        rdt_send(buffer, seq);
        seq = 1 - seq;
        memset(buffer, 0, DATA_SIZE);
    }

    fclose(fin);
    close(client_socket);
    printf("Client closed\n");
    return 0;
}