#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "../server/network.h"
#include "../shared/protocol.h"

void test_setup_server() {
    printf("Testing setup_server...\n");
    int port = 55565; // Changed port
    int sock = setup_server(port);
    if (sock == -1) {
        fprintf(stderr, "Failed to setup server on port %d\n", port);
        exit(1);
    }
    assert(sock > 0);
    close(sock);
    printf("setup_server passed.\n");
}

void test_connection() {
    printf("Testing connection...\n");
    int port = 55566; // Changed port
    int server_sock = setup_server(port);
    assert(server_sock > 0);

    pid_t pid = fork();
    if (pid == 0) {
        // Child: Connect to server
        sleep(1); // Give server time to accept
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
            perror("Connect failed");
            exit(1);
        }
        close(sock);
        exit(0);
    } else {
        // Parent: Accept connection
        struct sockaddr_in client_addr;
        int client_sock = accept_new_connection(server_sock, &client_addr);
        if (client_sock == -1) {
            perror("Accept failed");
            kill(pid, SIGKILL);
        }
        assert(client_sock > 0);
        close(client_sock);
        close(server_sock);
        wait(NULL);
    }
    printf("connection passed.\n");
}

int main() {
    test_setup_server();
    test_connection();
    printf("All network tests passed!\n");
    return 0;
}
