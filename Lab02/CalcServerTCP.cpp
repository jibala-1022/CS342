#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <thread>
#include <algorithm>

std::map<int, sockaddr_in> clientAddresses; // Store client addresses
std::map<int, std::thread> clientThreads;   // Store client threads

// Function to handle a client's messages and detect disconnects
void handleClient(int clientSocket) {
    char buffer[1024];
    int bytesRead;

    while (true) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            // Handle client disconnect
            std::cout << "Client " << clientSocket << " disconnected." << std::endl;
            clientAddresses.erase(clientSocket);
            clientThreads.erase(clientSocket);
            close(clientSocket);
            return;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Received from client " << clientSocket << ": " << buffer << std::endl;

        // Broadcast the message to all other clients
        for (const auto& pair : clientAddresses) {
            int otherClientSocket = pair.first;
            if (otherClientSocket != clientSocket) {
                send(otherClientSocket, buffer, bytesRead, 0);
            }
        }
    }
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);  // Change this port as needed
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding: " << strerror(errno) << std::endl;
        return 1;
    }

    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error listening: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Server listening on port 8080..." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == -1) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            return 1;
        }

        // Store the client address for P2P communication
        clientAddresses[clientSocket] = clientAddr;

        std::cout << "Client " << clientSocket << " connected." << std::endl;

        // Create a new thread to handle the client
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
        clientThreads[clientSocket] = std::move(clientThread);
    }

    // Join and close client threads and sockets (This part won't be reached)
    for (auto& pair : clientThreads) {
        pair.second.join();
        int clientSocket = pair.first;
        close(clientSocket);
    }

    // Close the server socket (This part won't be reached)
    close(serverSocket);

    return 0;
}
