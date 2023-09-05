#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <map>
#include <set>
using namespace std;

#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024

map<int, pair<string, thread*>> client_map;
set<string> client_set;

// Function to handle communication with a client
void handle_client(int client_socket) {
    char* buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));
    string client_name;

    while(true){
        // Send the response back to the client
        if (send(client_socket, "Enter username: ", strlen("Enter username: "), 0) == -1) {
            perror("[-] Sending failed\n");
            break;
        }
        // Receive data from the client
        if (recv(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
            perror("[-] Receiving failed\n");
            break;
        }
        if (strcmp(buffer, "/exit") == 0) {
            cout << "[-] Connection closed by client" << endl;
            break;
        }
        client_name = buffer;
        if(client_set.find(client_name) != client_set.end()){
            continue;
        }
        client_map[client_socket].first = client_name;
        client_set.insert(client_name);


        // Print the received message from the client
        cout << "\n[*] Client " << client_name << ": " << buffer;
        // sprintf(buffer, "%d", client_map.size()-1);
    }

    // while(true){
    //     // Receive data from the client
    //     if (recv(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
    //         perror("[-] Receiving failed\n");
    //         break;
    //     }

    //     // Print the received message from the client
    //     cout << "\n[*] Client " << client_map[client_socket].first << ": " << buffer;

    //     // For simplicity, let's just send back the same data as a response
    //     cout << "[*] Reply to Client " << client_map[client_socket].first << ": ";
    //     cin.getline(buffer, BUFFER_SIZE);

    //     // Send the response back to the client
    //     if (send(client_socket, buffer, strlen(buffer), 0) == -1) {
    //         perror("[-] Sending failed\n");
    //         break;
    //     }
    // }

    free(buffer);
    close(client_socket);
    client_set.erase(client_name);
    client_map.erase(client_socket);
    cout << "[-] Client " << client_map[client_socket].first << " disconnected" << endl;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr<<"[-] Usage: "<<argv[0]<<" <port>"<<endl;
        return 1;
    }
    int port = atoi(argv[1]);

    int server_socket, client_socket;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        cerr<<"[-] Socket creation failed"<<endl;
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Bind the server socket to the specified port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        cerr<<"[-] Binding failed"<<endl;
        return 1;
    }
    cout<<"[+] Binding successful"<<endl;

    // Start listening for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        cerr<<"[-] Listening failed"<<endl;
        return 1;
    }
    cout<<"[+] Server listening on port "<<port<<"..."<<endl;

    while (true) {
        // Accept incoming client connection
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_socket == -1) {
            cout<<"[-] Accepting connection failed"<<endl;
            continue;
        }

        if(client_map.size() >= MAX_CLIENTS){
            send(client_socket, "0", strlen("0"), 0);
            continue;
        }

        // Notify the client that it's accepted
        send(client_socket, "1", strlen("1"), 0);

        cout<<"[+] New client connected - "<<inet_ntoa(client_addr.sin_addr)<<" : "<<ntohs(client_addr.sin_port)<<endl;

        int client_id = ntohs(client_addr.sin_port);

        thread client_thread(handle_client, client_socket);

        client_map[client_socket] = make_pair(NULL, &client_thread);
    }

    for (auto& client : client_map) {
        client.second.second->join();
    }

    close(server_socket);
    cout << "[-] Server closed" << endl;
    return 0;
}
