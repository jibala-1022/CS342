# CS342

## Lab 01

### Task 1 - Handling Multiple Clients
Server:
```c
gcc tcp_server_1.c -o server
./server 8080
```
Client:
```c
gcc tcp_client_1.c -o client
./client 127.0.0.1 8080
```

### Task 2 - Server Chatroom
Server:
```c
gcc tcp_server_2.c -o server
./server 8080
```
Client:
```c
gcc tcp_client_2.c -o client
./client 127.0.0.1 8080
```

### Task 3 - Network Calculator

#### TCP
Server:
```c
gcc CalcServerTCP.c -o server
./server 8080
```
Client:
```c
gcc CalcClientTCP.c -o client
./client 127.0.0.1 8080
```

#### UDP
Server:
```c
gcc CalcServerUDP.c -o server
./server 8080
```
Client:
```c
gcc CalcClientUDP.c -o client
./client 127.0.0.1 8080
```

---
