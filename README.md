# CS342
---

## Lab 01

### Task 1 - Handling Multiple Clients
Server:
```c
gcc tcp_server_1.cpp -o server
./server 8080
```
Client:
```c
gcc tcp_client_1.cpp -o client
./client 127.0.0.1 8080
```

### Task 2 - Server Chatroom
Server:
```c
gcc tcp_server_2.cpp -o server
./server 8080
```
Client:
```c
gcc tcp_client_2.cpp -o client
./client 127.0.0.1 8080
```

### Task 3 - Network Calculator

#### TCP
Server:
```c
gcc CalcServerTCP.cpp -o server
./server 8080
```
Client:
```c
gcc CalcClientTCP.cpp -o client
./client 127.0.0.1 8080
```

#### UDP
Server:
```c
gcc CalcServerUDP.cpp -o server
./server 8080
```
Client:
```c
gcc CalcClientUDP.cpp -o client
./client 127.0.0.1 8080
```

---
