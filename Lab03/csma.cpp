#include <iostream>
#include <vector>
#include <random>
#include <ctime>

using namespace std;

#define SAMPLE_TIME 100000
#define NUM_NODES 5
#define MAX_BACKOFF 5
#define MAX_TRANSMISSION_ATTEMPTS 8
#define IDLE 0
#define BUSY 1
#define DIFS 1
#define MIN_FRAME_SIZE 2
#define MAX_FRAME_SIZE 4

int channel_status = IDLE;

class Node;
vector<Node*> transmitting_nodes;

class Node {
private:
    int id;
    int backoff;
    int transmission_attempts;
    int dropped_packets;
    int difs;
    int frame;
    int transmitting_frame;
    bool collided;
    bool in_transmission;
    int collisions;
    int successful_transmissions;
public:
    Node(int id) : id(id), backoff(0), collisions(0), successful_transmissions(0), transmission_attempts(0), dropped_packets(0), in_transmission(false), difs(DIFS) {
        frame = MIN_FRAME_SIZE + rand() % (MAX_FRAME_SIZE - MIN_FRAME_SIZE + 1);
        transmitting_frame = frame;
    }
    int getId() { return id; }
    int getCollisions() { return collisions; }
    int getTransmissionAttempts() { return transmission_attempts; }
    int getTotalTransmissions() { return successful_transmissions; }

    bool isCollided() { return collided; }
    void resetBackoff() { backoff = 0; }
    void setBinExpBackoff() {
        int N = (transmission_attempts < MAX_BACKOFF) ? transmission_attempts : MAX_BACKOFF;
        backoff = rand() % (1 << N);
    }
    void setDifs() { difs = DIFS; }
    void setCollided() { collided = true; }
    void setFrame() { frame = MIN_FRAME_SIZE + rand() % (MAX_FRAME_SIZE - MIN_FRAME_SIZE + 1); }
    bool isChannelIdle() { return channel_status == IDLE; }
    bool canTransmit() { return backoff == 0; }
    void transmit() {
        in_transmission = true;
        transmitting_frame = frame;
        cout << "Node " << id << " started transmitting. Packet size: " << transmitting_frame << endl;
        transmitting_nodes.push_back(this);
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
                    // difs--;
                }
            }
        }

    }

    void acked(bool collided_transmission) {
        transmission_attempts++;
        if(collided_transmission && transmission_attempts < MAX_TRANSMISSION_ATTEMPTS){
            collisions++;
            setBinExpBackoff();
            cout << "Node " << id << " collided during transmission. Retrying after backoff: " << backoff << endl;
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

void displayStatistics(vector<Node*>& nodes){
    cout << "\nSimulation Results:" << endl;
    for(Node* node : nodes){
        cout << "Node " << node->getId() << endl;
        cout << "Total Successful Transmissions: " << node->getTotalTransmissions() << endl;
        cout << "Total Collisions: " << node->getCollisions() << endl;
    }
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
