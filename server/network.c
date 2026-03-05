#include "network.h"

static int create_and_configure_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket failed");
        return -1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        close(sock);
        return -1;
    }
    return sock;
}

static int bind_and_listen(int sock, int port) {
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1) {
        perror("bind failed");
        return -1;
    }

    if (listen(sock, SOMAXCONN) == -1) {
        perror("listen failed");
        return -1;
    }
    return 0;
}

int setup_server(int port) {
    int sock = create_and_configure_socket();
    if (sock == -1) {
        return -1;
    }

    if (bind_and_listen(sock, port) == -1) {
        close(sock);
        return -1;
    }

    return sock;
}

int accept_new_connection(int server_sock, struct sockaddr_in *client_addr) {
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int client_sock = accept(server_sock, (struct sockaddr *)client_addr, &client_addr_len);
    if (client_sock == -1) {
        perror("accept failed");
        return -1;
    }
    return client_sock;
}
