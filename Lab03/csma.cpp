// g++ csmaca.cpp -o csmaca && ./csmaca
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <ctime>

using namespace std;

#define SAMPLE_TIME 10000
#define NUM_NODES 5
#define MAX_BACKOFF 5
#define MAX_TRANSMISSION_ATTEMPTS 8
#define FRAME_SIZE_MIN 2
#define FRAME_SIZE_MAX 4
#define IDLE 0
#define BUSY 1
#define DIFS 1
#define SIFS 1
#define ACK 1

#define NORMAL "\033[0m"
#define INVERT "\033[30;47m"
#define HEADING "\033[1;100m"
#define RED "\033[91m"
#define GREEN "\033[92m"
#define YELLOW "\033[93m"
#define UNDERLINE "\033[4m"

class Node;

class Channel {
private:
    static int status;
    static vector<Node*> transmitting_nodes;
public:
    static void transmission();
    friend class Node;
};
int Channel::status = IDLE;
vector<Node*> Channel::transmitting_nodes;

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
    Node(int id) : id(id), backoff(0), collisions(0), successful_transmissions(0), transmission_attempts(0), dropped_packets(0), n_backoffs(0), in_transmission(false), difs(DIFS) {
        frame = FRAME_SIZE_MIN + (double)rand() / RAND_MAX * (FRAME_SIZE_MAX - FRAME_SIZE_MIN + 1);
    }
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
    void setFrame() { frame = FRAME_SIZE_MIN + (double)rand() / RAND_MAX * (FRAME_SIZE_MAX - FRAME_SIZE_MIN + 1); }
    bool isChannelIdle() { return Channel::status == IDLE; }
    bool canTransmit() { return backoff == 0; }

    void attemptTransmission();
    void transmit();
    bool transmitting();
    void ack();
};

void Node::transmit() {
    in_transmission = true;
    transmitting_frame = frame + SIFS + ACK;
    cout << "Node " << id << " started transmitting. Packet size: " << transmitting_frame << endl;
    Channel::transmitting_nodes.push_back(this);
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
            cout << "Node " << id << " collided before transmission. Retrying after backoff: " << backoff << endl;
        }
        // else{
        //     if(difs == 0){
        //         // do nothing
        //     }
        //     else{
        //         // difs--;
        //     }
        // }
    }
}

void Node::ack() {
    transmission_attempts++;
    if(collided){
        collisions++;
        // n_backoffs++;
        if(transmission_attempts < MAX_TRANSMISSION_ATTEMPTS){
            setBinExpBackoff();
            cout << "Node " << id << " collided during transmission. Retrying after backoff: " << backoff << endl;
        }
        else{
            dropped_packets++;
            cout << "Node " << id << " collided during transmission. Packet dropped" << endl;
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
        cout << "Node " << id << " successfully transmitted." << endl;
    }
    setDifs();
    collided = false;
    in_transmission = false;
}


void Channel::transmission() {
    vector<Node*> unfinished_nodes;
    for(Node* node : transmitting_nodes){
        Channel::status = BUSY;
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
    transmitting_nodes = unfinished_nodes;
    if(unfinished_nodes.size() == 0){
        Channel::status = IDLE;
    }
}

void update(vector<Node*>& nodes){
    for(Node* node : nodes){
        node->attemptTransmission();
    }
    Channel::transmission();
}

void displayStatistics(vector<Node*>& nodes){
    cout << endl << right;
    cout << UNDERLINE << "Simulation Results" << NORMAL << endl;

    cout << HEADING << "Node ID                   | ";
    for(Node* node : nodes){
        cout << setw(8) << "Node " + to_string(node->getId()) << " | ";
    }
    cout << NORMAL << endl;
    cout << GREEN << "Successful Transmissions  | ";
    for(Node* node : nodes){
        cout << setw(8) << node->getSuccessfulTransmissions() << " | ";
    }
    cout << NORMAL << endl;
    cout << RED << "Collisions                | ";
    for(Node* node : nodes){
        cout << setw(8) << node->getCollisions() << " | ";
    }
    cout << NORMAL << endl;
    cout << YELLOW << "Backoffs                  | ";
    for(Node* node : nodes){
        cout << setw(8) << node->getBackoffs() << " | ";
    }
    cout << NORMAL << endl;
    cout << RED << "Dropped Packets           | ";
    for(Node* node : nodes){
        cout << setw(8) << node->getDroppedPackets() << " | ";
    }
    cout << NORMAL << endl;
    cout << left << endl;
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    vector<Node*> nodes;
    for (int i = 0; i < NUM_NODES; i++){
        Node* newNode = new Node(i + 1);
        nodes.push_back(newNode);
    }

    for(int i=0; i < SAMPLE_TIME; i++){
        cout << ">> " << i + 1 << endl;
        update(nodes);
    }

    displayStatistics(nodes);

    return 0;
}
