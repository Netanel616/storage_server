#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "../server/network.h"
#include "../server/server_logic.h"
#include "../shared/protocol.h"

void test_server_startup() {
    printf("Testing server startup...\n");
    int port = 55558;
    int sock = setup_server(port);
    assert(sock > 0);
    close(sock);
    printf("server startup passed.\n");
}

void test_client_connection() {
    printf("Testing client connection...\n");
    int port = 55559;
    int server_sock = setup_server(port);
    assert(server_sock > 0);

    pid_t pid = fork();
    if (pid == 0) {
        // Child: Connect to server
        sleep(1);
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
        assert(client_sock > 0);
        close(client_sock);
        close(server_sock);
        wait(NULL);
    }
    printf("client connection passed.\n");
}

int main() {
    test_server_startup();
    test_client_connection();
    printf("All main tests passed!\n");
    return 0;
}
