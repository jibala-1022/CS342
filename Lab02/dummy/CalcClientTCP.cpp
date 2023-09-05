#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <thread>
#include <algorithm>

std::vector<int> clientSockets; // Store client sockets

// Function to handle messages from a connected client
void handleClient(int clientSocket) {
    char buffer[1024];
    int bytesRead;

    while (true) {
        bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            // Handle client disconnect
            std::cout << "Client disconnected." << std::endl;
            close(clientSocket);
            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
            return;
        }

        buffer[bytesRead] = '\0';
        std::cout << "Received from client: " << buffer << std::endl;
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

    char message[1024];

    // Receive a list of existing clients from the server
    while (true) {
        int bytesRead = recv(clientSocket, message, sizeof(message), 0);
        if (bytesRead <= 0) {
            break;
        }
        message[bytesRead] = '\0';
        std::cout << "Connected to client: " << message << std::endl;

        // Connect to the existing clients
        sockaddr_in clientAddr;
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_port = htons(serverPort);
        clientAddr.sin_addr.s_addr = inet_addr(message);

        int existingClientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (existingClientSocket != -1) {
            if (connect(existingClientSocket, (struct sockaddr*)&clientAddr, sizeof(clientAddr)) != -1) {
                clientSockets.push_back(existingClientSocket);
                std::thread clientThread(handleClient, existingClientSocket);
                clientThread.detach();
            }
        }
    }

    // Accept connections from new clients
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == -1) {
        std::cerr << "Error creating socket: " << strerror(errno) << std::endl;
        return 1;
    }

    sockaddr_in listenAddr;
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port = htons(serverPort);
    listenAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr*)&listenAddr, sizeof(listenAddr)) == -1) {
        std::cerr << "Error binding: " << strerror(errno) << std::endl;
        return 1;
    }

    if (listen(listenSocket, 10) == -1) {
        std::cerr << "Error listening: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Client listening for incoming connections..." << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int newClientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (newClientSocket == -1) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            continue;
        }

        // Connect to the new client
        clientSockets.push_back(newClientSocket);
        std::thread clientThread(handleClient, newClientSocket);
        clientThread.detach();
    }

    // Close client sockets (This part won't be reached)
    for (int socket : clientSockets) {
        close(socket);
    }

    return 0;
}
