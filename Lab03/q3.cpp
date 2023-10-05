#include <iostream>
#include <cstring>
#include <ctime>
#include <cstdlib>
#include <unistd.h>
#include <thread>

using namespace std;

const int MAX_SEQ = 7;
const int FRAME_SIZE = 8;
const int TIMEOUT = 3; // Timeout in seconds

struct Frame {
    int seq;
    char data[FRAME_SIZE];
    int checksum;
};

bool isCorrupted(Frame frame) {
    // Simulate a 10% chance of corruption
    return rand() % 10 == 0;
}

bool isLost() {
    // Simulate a 10% chance of frame loss
    return rand() % 10 == 0;
}

Frame makeFrame(int seq, const char* data) {
    Frame frame;
    frame.seq = seq;
    strncpy(frame.data, data, FRAME_SIZE);
    frame.checksum = 0;
    for (int i = 0; i < FRAME_SIZE; i++) {
        frame.checksum += frame.data[i];
    }
    return frame;
}

void sender() {
    srand(time(0));
    int nextSeqNum = 0;
    char message[] = "Hello, Receiver!";
    
    while (true) {
        Frame frame = makeFrame(nextSeqNum, message);
        cout << "Sender: Sending frame with sequence number " << frame.seq << endl;
        
        if (isLost()) {
            cout << "Sender: Frame lost!" << endl;
        } else {
            if (isCorrupted(frame)) {
                cout << "Sender: Frame is corrupted!" << endl;
            } else {
                cout << "Sender: Frame received by receiver." << endl;
            }
        }
        
        nextSeqNum = (nextSeqNum + 1) % (MAX_SEQ + 1);
        sleep(TIMEOUT);
    }
}

void receiver() {
    srand(time(0));
    int expectedSeqNum = 0;
    
    while (true) {
        Frame frame;
        if (isLost()) {
            cout << "Receiver: Frame lost!" << endl;
        } else {
            cout << "Receiver: Waiting for frame with sequence number " << expectedSeqNum << endl;
            frame = makeFrame(expectedSeqNum, " ");
            if (isCorrupted(frame)) {
                cout << "Receiver: Frame is corrupted!" << endl;
            } else {
                cout << "Receiver: Frame received: " << frame.data << endl;
            }
        }
        
        // Acknowledge the frame
        cout << "Receiver: Sending acknowledgment for sequence number " << expectedSeqNum << endl;
        
        expectedSeqNum = (expectedSeqNum + 1) % (MAX_SEQ + 1);
    }
}

int main() {
    // Create two threads for sender and receiver
    thread senderThread(sender);
    thread receiverThread(receiver);

    // Join the threads to avoid the program from exiting
    senderThread.join();
    receiverThread.join();

    return 0;
}
