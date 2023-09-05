#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>

#define BUFFER_SIZE 1024

using namespace std;

// Function to handle receiving and displaying messages from the server
void receive_messages(int client_socket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            cerr << "[-] Connection to server closed" << endl;
            break;
        }

        buffer[bytes_received] = '\0';
        cout << "[Server]: " << buffer << endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "[-] Usage: " << argv[0] << " <server_ip_address> <port>" << endl;
        return 1;
    }

    int port = atoi(argv[2]);
    char *server_ip = argv[1];

    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create a socket for communication
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("[-] Socket creation failed");
        return 1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(port);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("[-] Connection failed");
        return 1;
    }

    // Receive initial response from the server
    recv(client_socket, buffer, strlen("0"), 0);
    if (buffer[0] == '0') {
        cerr << "[-] Server capacity reached" << endl;
        close(client_socket);
        return 1;
    }
    cout << "[+] Connected to server..." << endl << endl;

    // // Create a thread to receive messages from the server
    // thread receive_thread(receive_messages, client_socket);

    while (true) {
        if (recv(client_socket, buffer, strlen(buffer), 0) == -1) {
            cerr<<"[-] Receiving failed"<<endl;
            close(client_socket);
            return 1;
        }

        cout<<buffer<<endl;
        cin.getline(buffer, BUFFER_SIZE);

        // Send the message to the server
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            cerr<<"[-] Sending failed"<<endl;
            break;
        }

        // Check if the client wants to exit
        if (strcmp(buffer, "/exit") == 0) {
            cout << "[-] Connection closed by client" << endl;
            break;
        }
    }

    while (true) {
        cout << "[*] Enter message: ";
        cin.getline(buffer, BUFFER_SIZE);

        // Send the message to the server
        if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
            perror("[-] Sending failed");
            break;
        }

        // Check if the client wants to exit
        if (strcmp(buffer, "/exit") == 0) {
            cout << "[-] Connection closed by client" << endl;
            break;
        }
    }

    // Close the client socket
    close(client_socket);

    cout << "[-] Client closed" << endl;

    return 0;
}
