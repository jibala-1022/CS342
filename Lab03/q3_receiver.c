// gcc q3_receiver.c -o q3_receiver && ./q3_receiver
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define LOOPBACK "127.0.0.1"
#define PORT 8080
#define DATA_SIZE 20
#define DELAY_PROP_MIN 2
#define DELAY_PROP_MAX 3
#define P_CORRUPTED 0.5
#define MAX_CLIENTS 2

#define RED    91
#define GREEN  92
#define YELLOW 93

struct Packet {
    uint16_t seq;
    char data[DATA_SIZE];
    uint16_t checksum;
} sndpkt, rcvpkt;

int client_socket, server_socket;
struct sockaddr_in server_addr, client_addr;
socklen_t server_addr_len, client_addr_len;
struct timespec start_time = {0}, end_time = {0};
FILE* fout;

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
    int bytes_received = recv(client_socket, &rcvpkt, sizeof(struct Packet), 0);
    if (bytes_received == -1) {
        perror("Receiving failed");
        exit(1);
    }
    else if(bytes_received == 0) {
        printf("End of Communication\n");
        close(client_socket);
        close(server_socket);
        fclose(fout);
        exit(0);
    }

    tick();
    printf(">> Packet received - ");
}   


int main() {
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOOPBACK);
    server_addr.sin_port = htons(PORT);
    server_addr_len = sizeof(server_addr);
    client_addr_len = sizeof(client_addr);

    if (bind(server_socket, (struct sockaddr*)&server_addr, server_addr_len) == -1) {
        perror("Binding failed");
        exit(1);
    }

    if((fout = fopen("receiver.txt", "w")) == NULL){
        perror("File opening failed");
        exit(1);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Listening failed");
        exit(1);
    }

    printf("Server listening on port %d...\n", PORT);

    if((client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len)) == -1){
        perror("Connecting failed");
        exit(1);
    }

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    printf("Connected to client...\n");

    uint16_t seq = 0;

    while (1) {
        rdt_rcv();

        if(isCorrupted()){
            printf("Corrupted\n");
            print_pkt(&rcvpkt, RED);

            sndpkt.seq = 1 - seq;
            sndpkt.checksum = 0;
            sprintf(sndpkt.data, "ACK %d", 1 - seq);
        }
        else if(!isAck(seq)){
            printf("Old Packet\n");
            print_pkt(&rcvpkt, RED);

            sndpkt.seq = 1 - seq;
            sndpkt.checksum = 0;
            sprintf(sndpkt.data, "ACK %d", 1 - seq);
        }
        else{
            printf("ACKed\n");
            print_pkt(&rcvpkt, GREEN);
            fprintf(fout, "%s ", rcvpkt.data);
            fflush(fout);

            sndpkt.seq = seq;
            sndpkt.checksum = 1;
            sprintf(sndpkt.data, "ACK %d", seq);
            seq = 1 - seq;
        }

        sndpkt.checksum = calculateChecksum();
        udt_send();
    }

    fclose(fout);
    close(client_socket);
    printf("Client closed\n");
    close(server_socket);
    printf("Server closed\n");
    return 0;
}
