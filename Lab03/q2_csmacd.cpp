// g++ q2_csmacd.cpp -o q2_csmacd && ./q2_csmacd
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>
#include <ctime>

#define NORMAL    "\033[0m"
#define INVERT    "\033[30;47m"
#define HEADING   "\033[1;100m"
#define RED       "\033[91m"
#define GREEN     "\033[92m"
#define YELLOW    "\033[93m"
#define UNDERLINE "\033[4m"

static int sample_time;
static int max_backoff;
static int frame_size;

class Node;
enum Status { IDLE, BUSY };

Status channel_status = IDLE;
std::vector<Node*> nodes;
std::vector<Node*> transmitting_nodes;

class Node {
private:
    int id;
    int backoff;
    int transmission_attempts;
    int frame;
    bool in_transmission;
    int successful_transmissions;   
    int collisions;
    int n_backoffs;
public:
    Node(int id) : id(id), backoff(0), transmission_attempts(0), frame(frame_size), in_transmission(false), successful_transmissions(0), collisions(0), n_backoffs(0) {}
    int getId() { return id; }
    int getSuccessfulTransmissions() { return successful_transmissions; }
    int getCollisions() { return collisions; }
    int getBackoffs() { return n_backoffs; }

    bool canTransmit() { return backoff == 0; }
    bool isTransmitting() { return in_transmission; }

    void setFrame() { frame = frame_size; }
    void resetBackoff() { backoff = 0; }
    void setBinExpBackoff() {
        int N = (transmission_attempts < max_backoff) ? transmission_attempts : max_backoff;
        int K = (double)rand() / RAND_MAX * (1 << N);
        backoff = K * frame;
    }

    void attemptTransmission();
    void collided();
    void transmit();
    bool transmitting();
};


void Node::collided(){
    transmission_attempts++;
    n_backoffs++;
    collisions++;
    setBinExpBackoff();
    std::cout << RED << "Node " << id << " collided. Retrying after backoff: " << backoff << NORMAL << std::endl;
}

void Node::transmit(){
    in_transmission = true;
    channel_status = BUSY;
    std::cout << GREEN << "Node " << id << " is transmitting. Frame size: " << frame << NORMAL << std::endl;
}

bool Node::transmitting(){
    frame--;
    if(frame == 0){
        resetBackoff();
        setFrame();
        in_transmission = false;
        successful_transmissions++;
        transmission_attempts = 0;
        channel_status = IDLE;
        std::cout << GREEN << "Node " << id << " finished transmitting." << NORMAL << std::endl;
        return true;
    }
    return false;
}

void Node::attemptTransmission() {
    if(in_transmission){
        return;
    }
    if(backoff > 0){
        backoff--;
        return;
    }
    if(channel_status == IDLE){
        transmitting_nodes.push_back(this);
    }
    else{
        transmission_attempts++;
        n_backoffs++;
        setBinExpBackoff();
        std::cout << YELLOW << "Node " << id << " is ready but channel is busy. Retrying after backoff: " << backoff << NORMAL << std::endl;
    }
}


void update(){
    for(Node* node : nodes){
        node->attemptTransmission();
    }
    if(transmitting_nodes.size() > 1){
        for(Node* node : transmitting_nodes){
            node->collided();
        }
        transmitting_nodes.clear();
    }
    else if(transmitting_nodes.size() == 1){
        Node* transmitting_node = transmitting_nodes[0];
        if(transmitting_node->isTransmitting() == false){
            transmitting_node->transmit();
        }
        bool done = transmitting_node->transmitting();
        if(done){
            transmitting_nodes.clear();
        }
    }
}

void displayStatistics(){
    std::cout << std::endl << std::right;
    std::cout << UNDERLINE << "Simulation Results" << NORMAL << std::endl;
    std::cout << "Sample Time: " << sample_time <<std::endl;
    std::cout << "Frame Size: " << frame_size << std::endl;

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
    std::cout << std::left << std::endl;
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    int n_nodes;
    std::cout << HEADING << "  CSMA/CD Simulation  " << NORMAL << std::endl;
    std::cout << "Enter the number of nodes (eg. 5): ";
    std::cin >> n_nodes;
    std::cout << "Enter the time of simulation (eg. 100000): ";
    std::cin >> sample_time;
    std::cout << "Enter the maximum backoff multiplier (eg. 5): ";
    std::cin >> max_backoff;
    std::cout << "Enter the frame size (eg. 4): ";
    std::cin >> frame_size;

    for (int i = 0; i < n_nodes; i++){
        Node* newNode = new Node(i + 1);
        nodes.push_back(newNode);
    }

    for(int i = 0; i < sample_time; i++){
        std::cout << HEADING << " " << i + 1 << " " << NORMAL << std::endl;
        update();
    }

    displayStatistics();

    return 0;
}
