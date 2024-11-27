# Compilation Instructions

## server

```bash
gcc -o main main.c common/file_controller.c -lpthread -fopenmp
```

## client

```bash
gcc -o client remcpclient/client.c common/file_controller.c
```

## file checker

```bash
gcc -o checker file_checker.c
```
