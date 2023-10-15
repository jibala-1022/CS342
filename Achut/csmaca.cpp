// g++ csmaca.cpp -o csmaca && ./csmaca
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <ctime>

#define SAMPLE_TIME 10000
#define NUM_NODES 5
#define MAX_BACKOFF 5
#define MAX_TRANSMISSION_ATTEMPTS 8
#define FRAME_SIZE 4
#define DIFS 1
#define SIFS 1
#define ACK 1

#define NORMAL "\033[0m"
#define INVERT "\033[30;47m"
#define HEADING "\033[1;100m"
#define RED "\033[91m"
#define GREEN "\033[92m"
#define YELLOW "\033[93m"
#define BLUE "\033[94m"
#define UNDERLINE "\033[4m"

class Node;
enum Status { IDLE, BUSY };

Status channel_status = IDLE;
std::vector<Node*> nodes;
std::vector<Node*> transmitting_nodes;
int wasted_time = 0;

class Node {
private:
    int id;
    int backoff;
    int transmission_attempts;
    int difs;
    int frame;
    int transmitting_frame;
    bool collided;
    bool in_transmission;
    int successful_transmissions;
    int collisions;
    int n_backoffs;
    int dropped_packets;
public:
    Node(int id) : id(id), backoff(0), collisions(0), successful_transmissions(0), transmission_attempts(0), frame(FRAME_SIZE), dropped_packets(0), n_backoffs(0), in_transmission(false), difs(DIFS) {}
    int getId() { return id; }
    int getTransmissionAttempts() { return transmission_attempts; }
    int getSuccessfulTransmissions() { return successful_transmissions; }
    int getCollisions() { return collisions; }
    int getBackoffs() { return n_backoffs; }
    int getDroppedPackets() { return dropped_packets; }

    bool isCollided() { return collided; }
    void resetBackoff() { backoff = 0; }
    void setBinExpBackoff() {
        int N = (transmission_attempts < MAX_BACKOFF) ? transmission_attempts : MAX_BACKOFF;
        int K = (double)rand() / RAND_MAX * (1 << N);
        backoff = K * frame;
    }
    void setDifs() { difs = DIFS; }
    void setCollided() { collided = true; }
    void setFrame() { frame = FRAME_SIZE; }
    bool isChannelIdle() { return channel_status == IDLE; }
    bool canTransmit() { return backoff == 0; }

    void attemptTransmission();
    void transmit();
    bool transmitting();
    void ack();
};

void Node::transmit() {
    in_transmission = true;
    transmitting_frame = frame;
    std::cout << BLUE << "Node " << id << " started transmitting. Packet size: " << transmitting_frame << NORMAL << std::endl;
    transmitting_frame += SIFS + ACK;
    transmitting_nodes.push_back(this);
}

bool Node::transmitting() {
    if(difs == 0){
        transmitting_frame--;
        return transmitting_frame == 0;
    }
    else{
        difs--;
        return false;
    }
    return false;
}

void Node::attemptTransmission() {
    if(in_transmission){
        return;
    }
    if(isChannelIdle()){
        if(backoff == 0){
            if(difs == 0){
                transmit();
            }
            else{
                difs--;
            }
        }
        else{
            if(difs == 0){
                backoff--;
            }
            else{
                difs--;
            }
        }
    }
    else{
        if(backoff == 0){
            transmission_attempts++;
            n_backoffs++;
            setDifs();
            setBinExpBackoff();
            std::cout << YELLOW << "Node " << id << " is ready but channel is busy. Retrying after backoff: " << backoff << NORMAL << std::endl;
        }
    }
}

void Node::ack() {
    transmission_attempts++;
    if(collided){
        collisions++;
        n_backoffs++;
        if(transmission_attempts < MAX_TRANSMISSION_ATTEMPTS){
            setBinExpBackoff();
            std::cout << RED << "Node " << id << " collided during transmission. Attempt: " << transmission_attempts << "/" << MAX_TRANSMISSION_ATTEMPTS << ". Retrying after backoff: " << backoff << NORMAL << std::endl;
        }
        else{
            dropped_packets++;
            std::cout << RED << "Node " << id << " collided during transmission. Attempt: " << MAX_TRANSMISSION_ATTEMPTS << "/" << MAX_TRANSMISSION_ATTEMPTS << ". Packet dropped" << NORMAL << std::endl;
            transmission_attempts = 0;
            setFrame();
            resetBackoff();
        }
    }
    else{
        transmission_attempts = 0;
        successful_transmissions++;
        setFrame();
        resetBackoff();
        std::cout << GREEN << "Node " << id << " successfully transmitted." << NORMAL << std::endl;
    }
    setDifs();
    collided = false;
    in_transmission = false;
}


void transmission() {
    std::vector<Node*> unfinished_nodes;
    for(Node* node : transmitting_nodes){
        channel_status = BUSY;
        bool done = node->transmitting();
        if(transmitting_nodes.size() > 1){
            node->setCollided();
        }
        if(done){
            node->ack();
        }
        else{
            unfinished_nodes.push_back(node);
        }
    }
    if(channel_status == IDLE){
        wasted_time++;
    }
    transmitting_nodes = unfinished_nodes;
    if(unfinished_nodes.size() == 0){
        channel_status = IDLE;
    }
}

void update(){
    for(Node* node : nodes){
        node->attemptTransmission();
    }
    transmission();
}

void displayStatistics(){
    std::cout << std::endl << std::right;
    std::cout << UNDERLINE << "Simulation Results" << NORMAL << std::endl;
    std::cout << "Sample Time: " << SAMPLE_TIME <<std::endl;
    std::cout << "Frame Size: " << FRAME_SIZE << std::endl;

    std::cout << HEADING << " Node ID                   | ";
    for(Node* node : nodes){
        std::cout << std::setw(8) << "Node " + std::to_string(node->getId()) << " | ";
    }
    std::cout << NORMAL << std::endl;
    std::cout << GREEN << " Successful Transmissions  | ";
    for(Node* node : nodes){
        std::cout << std::setw(8) << node->getSuccessfulTransmissions() << " | ";
    }
    std::cout << NORMAL << std::endl;
    std::cout << RED << " Collisions                | ";
    for(Node* node : nodes){
        std::cout << std::setw(8) << node->getCollisions() << " | ";
    }
    std::cout << NORMAL << std::endl;
    std::cout << YELLOW << " Backoffs                  | ";
    for(Node* node : nodes){
        std::cout << std::setw(8) << node->getBackoffs() << " | ";
    }
    std::cout << NORMAL << std::endl;
    std::cout << RED << " Dropped Packets           | ";
    for(Node* node : nodes){
        std::cout << std::setw(8) << node->getDroppedPackets() << " | ";
    }
    std::cout << NORMAL << std::endl;
    std::cout << RED << "Total Wasted Slots: " << wasted_time << NORMAL << std::endl;
    std::cout << std::left << std::endl;
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    int n_nodes;
    std::cout << HEADING << "  CSMA/CA Simulation  " << NORMAL << std::endl;
    std::cout << "Enter the number of nodes: ";
    std::cin >> n_nodes;

    for (int i = 0; i < n_nodes; i++){
        Node* newNode = new Node(i + 1);
        nodes.push_back(newNode);
    }

    for(int i=0; i < SAMPLE_TIME; i++){
        std::cout << HEADING << " " << i + 1 << " " << NORMAL << std::endl;
        update();
    }

    displayStatistics();

    return 0;
}
