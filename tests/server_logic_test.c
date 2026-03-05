#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "../server/server_logic.h"
#include "../server/network.h"
#include "../shared/protocol.h"

void test_receive_and_save_file() {
    printf("Testing receive_and_save_file...\n");
    int port = 55557;
    int server_sock = setup_server(port);
    assert(server_sock > 0);

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

        char *filename = "test_file.txt";
        char *content = "Hello, world!";
        uint64_t content_len = strlen(content);
        uint16_t name_len = strlen(filename);

        FileHeader header;
        header.type = 1;
        header.data_size = htobe64(content_len);
        header.name_size = htobe16(name_len);

        send(sock, &header, sizeof(header), 0);
        send(sock, filename, name_len, 0);
        send(sock, content, content_len, 0);

        close(sock);
        exit(0);
    } else {
        // Parent: Accept and receive file
        struct sockaddr_in client_addr;
        int client_sock = accept_new_connection(server_sock, &client_addr);
        assert(client_sock > 0);

        FileHeader header;
        recv(client_sock, &header, sizeof(header), 0);

        int result = receive_and_save_file(client_sock, &header);
        assert(result == 0);

        // Verify file content
        FILE *fp = fopen("test_file.txt", "r");
        assert(fp != NULL);
        char buffer[100];
        fgets(buffer, sizeof(buffer), fp);
        assert(strcmp(buffer, "Hello, world!") == 0);
        fclose(fp);
        remove("test_file.txt");

        close(client_sock);
        close(server_sock);
        wait(NULL);
    }
    printf("receive_and_save_file passed.\n");
}

int main() {
    test_receive_and_save_file();
    printf("All server logic tests passed!\n");
    return 0;
}
