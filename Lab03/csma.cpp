#include <iostream>
#include <vector>
#include <random>
#include <ctime>

using namespace std;

#define SAMPLE_TIME 1000
#define NUM_NODES 5
#define MAX_BACKOFF 5
#define MAX_TRANSMISSION_ATTEMPTS 1
#define IDLE 0
#define BUSY 1
#define DIFS 0
#define MIN_FRAME_SIZE 1
#define MAX_FRAME_SIZE 1

int total_transmissions = 0;
int total_collisions = 0;
int channel_status = IDLE;

class Node;
vector<Node*> transmitting_nodes;

class Node {
private:
    int id;
    int backoff;
    int collisions;
    int transmission_attempts;
    int dropped_packets;
    int difs;
    int frame;
    int transmitting_frame;
    bool collided;
    bool in_transmission;
public:
    Node(int id) : id(id), backoff(0), collisions(0), transmission_attempts(0), dropped_packets(0), in_transmission(false), difs(DIFS) {
        frame = MIN_FRAME_SIZE + rand() % (MAX_FRAME_SIZE - MIN_FRAME_SIZE + 1);
        transmitting_frame = frame;
    }
    int getId() { return id; }
    int getCollisions() { return collisions; }
    int getTransmissionAttempts() { return transmission_attempts; }
    bool isCollided() { return collided; }
    void resetBackoff() { backoff = 0; }
    void setBinExpBackoff() {
        // int N = MAX_BACKOFF;
        int N = (transmission_attempts < MAX_BACKOFF) ? transmission_attempts : MAX_BACKOFF;
        backoff = rand() % (1 << N);
    }
    void setDifs() { difs = DIFS; }
    void setCollided() { collided = true; }
    void setFrame() { frame = MIN_FRAME_SIZE + rand() % (MAX_FRAME_SIZE - MIN_FRAME_SIZE + 1); }
    bool isChannelIdle() { return channel_status == IDLE; }
    bool canTransmit() { return backoff == 0; }
    // bool canTransmit() { return transmission_attempts < MAX_TRANSMISSION_ATTEMPTS; }
    void transmit() {
        cout << "Node " << id << " started transmitting." << endl;
        in_transmission = true;
        transmitting_nodes.push_back(this);
        if(difs > 0) difs--;
        transmitting_frame = frame;
    }
    bool transmitting() {
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

    void attemptTransmission() {
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
                setDifs();
                setBinExpBackoff();
                cout << "Node " << id << " collided before transmission. Retrying after backoff: " << backoff << endl;
            }
            else{
                if(difs == 0){
                    // do nothing
                }
                else{
                    difs--;
                }
            }
        }

    }

    void acked(bool collided_transmission) {
        if(collided_transmission){
            transmission_attempts++;
            collisions++;
            total_collisions++;
            collided = false;
            in_transmission = false;
            setBinExpBackoff();
            cout << "Node " << id << " collided during transmission. Retrying after backoff: " << backoff << endl;
        }
        else{
            transmission_attempts = 0;
            total_transmissions++;
            collided = false;
            in_transmission = false;
            setFrame();
            resetBackoff();
            cout << "Node " << id << " successfully transmitted." << endl;
        }
    }

};

void linkTransmission() {
    bool done;
    int num_nodes = transmitting_nodes.size();

    vector<Node*> unfinished_nodes;
    for(Node* node : transmitting_nodes){
        channel_status = BUSY;
        done = node->transmitting();
        if(num_nodes > 1){
            node->setCollided();
        }
        if(done){
            node->acked(node->isCollided());
        }
        else{
            unfinished_nodes.push_back(node);
        }
    }
    transmitting_nodes = unfinished_nodes;
    if(unfinished_nodes.size() == 0){
        channel_status = IDLE;
    }
}

void update(vector<Node*>& nodes){
    for(Node* node : nodes){
        node->attemptTransmission();
    }

    linkTransmission();
}

void displayStatistics(){
    cout << "\nSimulation Results:" << endl;
    cout << "Total Successful Transmissions: " << total_transmissions << endl;
    cout << "Total Collisions: " << total_collisions << endl;
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

    displayStatistics();

    return 0;
}
