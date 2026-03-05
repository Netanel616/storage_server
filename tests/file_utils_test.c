#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "../shared/file_utils.h"
#include "../server/network.h"

void test_send_recv_all() {
    printf("Testing send_all and recv_all...\n");
    int port = 55560;
    int server_sock = setup_server(port);
    assert(server_sock > 0);

    pid_t pid = fork();
    if (pid == 0) {
        // Child: Connect and send data
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

        char *msg = "Hello, World!";
        int res = send_all(sock, msg, strlen(msg));
        assert(res == 0);
        close(sock);
        exit(0);
    } else {
        // Parent: Accept and receive data
        struct sockaddr_in client_addr;
        int client_sock = accept_new_connection(server_sock, &client_addr);
        assert(client_sock > 0);

        char buffer[20];
        memset(buffer, 0, sizeof(buffer));
        int res = recv_all(client_sock, buffer, 13); // "Hello, World!" is 13 chars
        assert(res == 0);
        assert(strcmp(buffer, "Hello, World!") == 0);

        close(client_sock);
        close(server_sock);
        wait(NULL);
    }
    printf("send_all and recv_all passed.\n");
}

void test_send_file() {
    printf("Testing send_file...\n");
    int port = 55561;
    int server_sock = setup_server(port);
    assert(server_sock > 0);

    // Create a dummy file to send
    char *filename = "test_send.txt";
    char *content = "This is a test file content.";
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "%s", content);
    fclose(fp);

    pid_t pid = fork();
    if (pid == 0) {
        // Child: Connect and send file
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

        int res = send_file(sock, filename, 1);
        assert(res == 0);
        close(sock);
        exit(0);
    } else {
        // Parent: Accept and receive file manually to verify protocol
        struct sockaddr_in client_addr;
        int client_sock = accept_new_connection(server_sock, &client_addr);
        assert(client_sock > 0);

        FileHeader header;
        recv_all(client_sock, &header, sizeof(header));

        assert(header.type == 1);
        uint64_t data_size = be64toh(header.data_size);
        uint16_t name_size = be16toh(header.name_size);

        assert(data_size == strlen(content));
        assert(name_size == strlen(filename));

        char name_buffer[100];
        recv_all(client_sock, name_buffer, name_size);
        name_buffer[name_size] = '\0';
        assert(strcmp(name_buffer, filename) == 0);

        char content_buffer[100];
        recv_all(client_sock, content_buffer, data_size);
        content_buffer[data_size] = '\0';
        assert(strcmp(content_buffer, content) == 0);

        close(client_sock);
        close(server_sock);
        wait(NULL);

        // Cleanup
        remove(filename);
    }
    printf("send_file passed.\n");
}

int main() {
    test_send_recv_all();
    test_send_file();
    printf("All file_utils tests passed!\n");
    return 0;
}
