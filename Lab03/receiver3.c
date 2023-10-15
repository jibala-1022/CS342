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
#define DATA_SIZE 20
#define DELAY_PROP_MIN 1
#define DELAY_PROP_MAX 1
#define P_LOST 0.7
#define P_CORRUPTED 0.5

#define RED 91
#define GREEN 92
#define YELLOW 93

struct Packet {
    int seq;
    char data[DATA_SIZE];
    int checksum;
    int lost;
} sndpkt, rcvpkt;

int client_socket, server_socket;
struct sockaddr_in server_addr, client_addr;
socklen_t server_addr_len, client_addr_len;
struct timespec start_time = {0}, end_time = {0};
FILE* fout;
int started = 0;

void tick(){
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    int time_elapsed = end_time.tv_sec - start_time.tv_sec;
    printf("\033[1;100m %3d \033[0m\n", time_elapsed);
}

int isLost(){ return (double)rand() / RAND_MAX < P_LOST; }
int isCorrupted(){ return (double)rand() / RAND_MAX < P_CORRUPTED; }
int isAck(int seq){ return rcvpkt.seq == seq; }

void print_pkt(struct Packet* packet, int color){
    printf("    \033[%dm SEQ = %d \033[0m\n", color, packet->seq);
    printf("    \033[%dm DATA = %s \033[0m\n", color, packet->data);
    printf("    \033[%dm CHECKSUM = %d \033[0m\n", color, packet->checksum);
}

void udt_send(){
    pid_t pid;

    tick();
    printf("Packet sent\n");
    print_pkt(&sndpkt, YELLOW);
    
    if((pid = fork()) == -1){
        perror("Process creation failed");
        exit(1);
    }
    else if(pid == 0){
        int delay_seconds = DELAY_PROP_MIN + (double)rand() / RAND_MAX * (DELAY_PROP_MAX - DELAY_PROP_MIN + 1);
        sleep(delay_seconds);

        tick();
        if(isLost()){
            printf("Packet Lost\n");
            sndpkt.lost = 1;
        }
        else{
            sndpkt.lost = 0;
        }
        if (sendto(server_socket, &sndpkt, sizeof(struct Packet), 0, (struct sockaddr*)&client_addr, client_addr_len) == -1) {
            perror("Sending failed");
            close(client_socket);
            close(server_socket);
            fclose(fout);
            exit(1);
        }
        printf("Packet reached\n");
        close(client_socket);
        close(server_socket);
        fclose(fout);
        exit(0);
    }
}

void rdt_rcv(){
    memset(&rcvpkt, 0, sizeof(struct Packet));
    int bytes_received;
    if ((bytes_received = recvfrom(server_socket, &rcvpkt, sizeof(struct Packet), 0, (struct sockaddr*)&client_addr, &client_addr_len)) == -1) {
        perror("Receiving failed");
        exit(1);
    }

    if(!started){
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        started = 1;
    }
    tick();
    printf("Packet received\n");
}   


int main() {
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
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

    printf("Server listening on port %d...\n", PORT);

    int seq = 0;

    while (1) {
        rdt_rcv();

        if(rcvpkt.lost) continue;

        if(!isCorrupted() && isAck(seq)){
            print_pkt(&rcvpkt, GREEN);
            fprintf(fout, "%s ", rcvpkt.data);
            fflush(fout);

            sndpkt.seq = seq;
            sndpkt.checksum = 1;
            sprintf(sndpkt.data, "ACK %d", seq);
            seq = 1 - seq;
        }
        else{
            print_pkt(&rcvpkt, RED);

            sndpkt.seq = 1 - seq;
            sndpkt.checksum = 0;
            sprintf(sndpkt.data, "ACK %d", 1 - seq);
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
