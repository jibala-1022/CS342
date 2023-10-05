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
#define PORT 8088
#define DATA_SIZE 20
#define DELAY_PROP_MIN 1
#define DELAY_PROP_MAX 1
#define P_LOST 0
#define P_CORRUPTED 0

struct Packet {
    int seq;
    char data[DATA_SIZE];
    int checksum;
} sndpkt, rcvpkt;

int client_socket, server_socket;
struct sockaddr_in server_addr, client_addr;
socklen_t server_addr_len, client_addr_len;
static struct timespec start_time = {0}, end_time = {0};
FILE* fout;

void tick(){
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    int time_elapsed = end_time.tv_sec - start_time.tv_sec;
    printf(">> %3d: ", time_elapsed);
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
        if (sendto(server_socket, &sndpkt, sizeof(struct Packet), 0, (struct sockaddr*)&client_addr, client_addr_len) == -1) {
            perror("Sending failed");
            return;
        }
        printf("Packet reached\n");
        exit(0);
    }
}

void rdt_rcv(){
    memset(&rcvpkt, 0, sizeof(struct Packet));
    if (recvfrom(server_socket, &rcvpkt, sizeof(struct Packet), 0, (struct sockaddr*)&client_addr, &client_addr_len) == -1) {
        perror("Receiving failed");
        return;
    }
    tick();
    printf("Packet received\n");
    print_pkt(&rcvpkt);
}

int main() {
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(LOOPBACK);
    server_addr.sin_port = htons(PORT);
    server_addr_len = sizeof(server_addr);
    client_addr_len = sizeof(client_addr);

    if (bind(server_socket, (struct sockaddr*)&server_addr, server_addr_len) == -1) {
        perror("Binding failed");
        return 1;
    }

    if((fout = fopen("receiver.txt", "w")) == NULL){
        perror("File opening failed");
        return 1;
    }

    char buffer[DATA_SIZE];
    int seq = 0;
    strcpy(sndpkt.data, "ACK");
    sndpkt.checksum = 0;
    rcvpkt.seq = seq;
    rcvpkt.checksum = 0;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    tick();
    printf("Server listening on port %d...\n", PORT);

    while (1) {
        rdt_rcv();

        if(!isCorrupted() && isAck(seq)){
            sndpkt.seq = seq;
            seq = 1 - seq;
            // fputs(buffer, fout);
            fprintf(stdout, "%s ", rcvpkt.data);
            fprintf(fout, "%s ", rcvpkt.data);
        }
        else{
            sndpkt.seq = 1 - seq;
        }
        udt_send();
    }

    fclose(fout);
    close(client_socket);
    printf("Client closed\n");
    close(server_socket);
    printf("Server closed\n");
    return 0;
}
