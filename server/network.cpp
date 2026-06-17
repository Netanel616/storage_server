#include "network.h"

#include <cerrno>
#include <cstring>
#include <iostream>

TcpServer::TcpServer(int port) : sock_(-1), port_(port) {}

TcpServer::~TcpServer() {
    close_socket();
}

TcpServer::TcpServer(TcpServer&& other) noexcept : sock_(other.sock_), port_(other.port_) {
    other.sock_ = -1;
}

TcpServer& TcpServer::operator=(TcpServer&& other) noexcept {
    if (this != &other) {
        close_socket();
        sock_ = other.sock_;
        port_ = other.port_;
        other.sock_ = -1;
    }
    return *this;
}

bool TcpServer::create_and_configure_socket() {
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == -1) {
        std::cerr << "socket failed: " << std::strerror(errno) << std::endl;
        return false;
    }

    int opt = 1;
    if (setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "setsockopt failed: " << std::strerror(errno) << std::endl;
        close_socket();
        return false;
    }
    return true;
}

bool TcpServer::bind_and_listen() {
    struct sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port_);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock_, reinterpret_cast<struct sockaddr*>(&local_addr), sizeof(local_addr)) == -1) {
        std::cerr << "bind failed on port " << port_ << ": " << std::strerror(errno) << std::endl;
        return false;
    }

    if (listen(sock_, SOMAXCONN) == -1) {
        std::cerr << "listen failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool TcpServer::start() {
    if (!create_and_configure_socket()) {
        return false;
    }
    if (!bind_and_listen()) {
        close_socket();
        return false;
    }
    return true;
}

int TcpServer::accept_client(std::string& out_client_ip, uint16_t& out_client_port) {
    struct sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);
    int client_sock = accept(sock_, reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);
    if (client_sock == -1) {
        std::cerr << "accept failed: " << std::strerror(errno) << std::endl;
        return -1;
    }
    out_client_ip = inet_ntoa(client_addr.sin_addr);
    out_client_port = ntohs(client_addr.sin_port);
    return client_sock;
}

void TcpServer::close_socket() {
    if (sock_ != -1) {
        close(sock_);
        sock_ = -1;
    }
}

extern "C" int setup_server(int port) {
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

    struct sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&local_addr), sizeof(local_addr)) == -1) {
        perror("bind failed");
        close(sock);
        return -1;
    }

    if (listen(sock, SOMAXCONN) == -1) {
        perror("listen failed");
        close(sock);
        return -1;
    }

    return sock;
}

extern "C" int accept_new_connection(int server_sock, struct sockaddr_in* client_addr) {
    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    int client_sock = accept(server_sock, reinterpret_cast<struct sockaddr*>(client_addr), &client_addr_len);
    if (client_sock == -1) {
        perror("accept failed");
        return -1;
    }
    return client_sock;
}
