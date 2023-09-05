#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <thread>
#include <algorithm>
#include <string>
#include <mutex>

std::map<int, sockaddr_in> clientAddresses; // Store client addresses
std::map<int, std::thread> clientThreads;   // Store client threads
std::mutex mutex; // Mutex for synchronization

// Function to handle a client's messages and detect disconnects
void handleClient(int clientSocket) {
    char buffer[1024];
    int bytesRead;

    while (true) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            // Handle client disconnect
            std::cout << "Client " << clientSocket << " disconnected." << std::endl;
            close(clientSocket);

            // Lock the mutex to safely remove client information
            std::lock_guard<std::mutex> lock(mutex);
            clientAddresses.erase(clientSocket);
            clientThreads.erase(clientSocket);

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

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int serverPort = std::atoi(argv[1]);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);  // Port from command line argument
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Error binding: " << strerror(errno) << std::endl;
        return 1;
    }

    if (listen(serverSocket, 10) == -1) {
        std::cerr << "Error listening: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Server listening on port " << serverPort << "..." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == -1) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            continue;
        }

        // Lock the mutex to safely add client information
        std::lock_guard<std::mutex> lock(mutex);
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
