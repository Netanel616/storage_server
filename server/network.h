#ifndef NETWORK_H
#define NETWORK_H

#include "../shared/protocol.h"

#ifdef __cplusplus
#include <string>

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;
    TcpServer(TcpServer&& other) noexcept;
    TcpServer& operator=(TcpServer&& other) noexcept;

    bool start();

    int accept_client(std::string& out_client_ip, uint16_t& out_client_port);

    int fd() const { return sock_; }
    int port() const { return port_; }

private:
    int sock_;
    int port_;

    bool create_and_configure_socket();
    bool bind_and_listen();
    void close_socket();
};

extern "C" {
#endif

int setup_server(int port);
int accept_new_connection(int server_sock, struct sockaddr_in* client_addr);

#ifdef __cplusplus
}
#endif

#endif
