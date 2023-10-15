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
#define DELAY_PROP_MIN 1
#define DELAY_PROP_MAX 1
#define P_LOST 0
#define P_CORRUPTED 0.5
#define TIMEOUT 5

#define RED 91
#define GREEN 92
#define YELLOW 93

struct Packet {
    int seq;
    char data[DATA_SIZE];
    int checksum;
    int lost;
} sndpkt, rcvpkt;

int client_socket;
struct sockaddr_in server_addr;
socklen_t server_addr_len;
struct itimerval timer;
struct timespec start_time = {0}, end_time = {0};
FILE* fin;

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
        printf("Prop delay %ds\n", delay_seconds);
        sleep(delay_seconds);
        tick();

        if(isLost()){
            printf("Packet Lost\n");
            sndpkt.lost = 1;
        }
        else{
            sndpkt.lost = 0;
        }
        if (sendto(client_socket, &sndpkt, sizeof(struct Packet), 0, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            perror("Sending failed");
            close(client_socket);
            fclose(fin);
            exit(1);
        }
        printf("Packet reached\n");
        close(client_socket);
        fclose(fin);
        exit(0);
    }
}

void rdt_rcv(){
    memset(&rcvpkt, 0, sizeof(struct Packet));
    int bytes_received;
    while(1){
        bytes_received = recvfrom(client_socket, (void*)&rcvpkt, sizeof(struct Packet), 0, (struct sockaddr*)&server_addr, &server_addr_len);
        if(bytes_received == -1) {
            if(errno == EINTR){
                printf("interrupted\n");
                continue;
            }
            else{
                perror("Receiving failed");
                exit(1);
            }
        }
        break;
    }
    tick();
    printf("Packet received\n");
}

void rdt_send(char* buffer, int seq){
    struct sigaction sa;
    sa.sa_handler = udt_send;
    sa.sa_flags = 0;    
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGALRM, &sa, NULL) == -1){
        perror("sigaction");
        exit(1);
    }

    timer.it_value.tv_sec = TIMEOUT;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = TIMEOUT;
    timer.it_interval.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(1);
    }
    // alarm(TIMEOUT);

    int checksum = 0;
    sndpkt.seq = seq;
    strcpy(sndpkt.data, buffer);
    sndpkt.checksum = checksum;
    
    while(1){
        udt_send();
        rdt_rcv();
        if(rcvpkt.lost) continue;
        if(isCorrupted()){
            printf("Corrupted\n");
            print_pkt(&rcvpkt, RED);
            continue;
        }
        if(isAck(seq)){
            printf("ACKed\n");
            print_pkt(&rcvpkt, GREEN);
            break;
        }
    }
}

int main() {
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
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

    printf("Connected to server...\n");

    char buffer[DATA_SIZE];
    int seq = 0;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    // tick();

    int itemsRead;
    while ((itemsRead = fscanf(fin, "%s", buffer)) == 1) {
        rdt_send(buffer, seq);
        seq = 1 - seq;
    }

    fclose(fin);
    close(client_socket);
    printf("Client closed\n");
    return 0;
}