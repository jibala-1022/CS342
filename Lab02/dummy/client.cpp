#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <mutex>

std::mutex mutex; // Mutex for synchronization

// Function to receive and display messages from the server
void receiveMessages(int clientSocket) {
    char buffer[1024];
    int bytesRead;

    while (true) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            // Handle server disconnect or error
            std::cerr << "Server disconnected or encountered an error." << std::endl;
            close(clientSocket);
            exit(1);
        }

        buffer[bytesRead] = '\0';
        std::cout << "Received from server: " << buffer << std::endl;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
        return 1;
    }

    const char* serverIP = argv[1];
    int serverPort = std::atoi(argv[2]);

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP);

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error connecting to server: " << strerror(errno) << std::endl;
        return 1;
    }

    std::string message;

    // Create a thread to receive and display messages
    std::thread receiveThread(receiveMessages, clientSocket);

    // Send messages to the server
    while (true) {
        std::cout << "Enter a message to send (or type 'exit' to quit): ";
        std::getline(std::cin, message);

        if (message == "exit") {
            break;
        }

        int bytesSent = send(clientSocket, message.c_str(), message.length(), 0);
        if (bytesSent == -1) {
            std::cerr << "Error sending message to server: " << strerror(errno) << std::endl;
            break;
        }
    }

    // Join the receive thread (This part won't be reached)
    receiveThread.join();

    // Close the client socket
    close(clientSocket);

    return 0;
}
