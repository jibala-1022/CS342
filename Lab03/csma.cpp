#include <iostream>
#include <vector>
#include <random>
#include <ctime>

using namespace std;

#define NUM_NODES 5
#define MAX_BACKOFF 10
#define MAX_TRANSMISSION_ATTEMPTS 1

int randomBackoff() { return rand() % MAX_BACKOFF; }

class Node {
private:
    int id;
    int backoff;
    int collisions;
    int transmission_attempts;
    int successful_transmissions;
public:
    Node(int id) : id(id), backoff(0), collisions(0), transmission_attempts(0), successful_transmissions(0) {}
    int getId() { return id; }
    int getBinExpBackoff() { return backoff; }
    int getCollisions() { return collisions; }
    int getTransmissionAttempts() { return transmission_attempts; }
    int getSuccessfulTransmissions() { return successful_transmissions; }
    void resetBackoffs() { backoff = 0; }
    bool canTransmit() { return transmission_attempts < MAX_TRANSMISSION_ATTEMPTS; }

    bool isChannelClear(vector<Node>& nodes) {
        for (Node& node : nodes) {
            if (node.id != id && node.backoff == 0) {
                return false;
            }
        }
        return true;
    }

    void transmit(vector<Node>& nodes) {
        if (backoff == 0) {
            if (isChannelClear(nodes)) {
                cout << "Node " << id << " is transmitting." << endl;
                successful_transmissions++;
                resetBackoffs();
            }
            else {
                collisions++;
                backoffs = randomBackoff();
                cout << "Node " << id << " collided. Retrying after backoff: " << backoffs << endl;
            }
        }
        else {
            backoffs--;
        }
    }
};

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    vector<Node> nodes;
    for (int i = 0; i < NUM_NODES; i++) {
        nodes.push_back(Node(i));
    }

    int totalTransmissions = 0;
    int totalCollisions = 0;

    for (int timeStep = 0; timeStep < 10; timeStep++) {
        cout << ">> " << timeStep << endl;
        for (Node& node : nodes) {
            if (node.canTransmit()) {
                node.transmit(nodes);
            }
        }
    }

    for (Node& node : nodes) {
        totalTransmissions += node.getSuccessfulTransmissions();
        totalCollisions += node.getCollisions();
    }

    cout << "\nSimulation Results:" << endl;
    cout << "Total Successful Transmissions: " << totalTransmissions << endl;
    cout << "Total Collisions: " << totalCollisions << endl;

    return 0;
}
