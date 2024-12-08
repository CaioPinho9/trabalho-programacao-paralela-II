# Compilation Instructions

## server

```bash
gcc -o server remcpserv/server.c -lpthread -fopenmp common/file_controller/file_controller.c common/socket/socket.c
```

## client

```bash
gcc -o client remcpclient/client.c common/file_controller/file_controller.c common/socket/socket.c
```

## file checker

```bash
gcc -o checker file_checker.c
```
