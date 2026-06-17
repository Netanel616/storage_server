#include "network.h"
#include "server_logic.h"
#include "../shared/protocol.h"

#include <atomic>
#include <csignal>
#include <iostream>
#include <thread>

namespace {
std::atomic<bool> g_running{true};

void handle_signal(int /*signum*/) {
    g_running.store(false);
}
}

int main(int argc, char* argv[]) {
    int port = SERVER_PORT;
    if (argc >= 2) {
        try {
            port = std::stoi(argv[1]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid port '" << argv[1] << "': " << e.what() << std::endl;
            return 1;
        }
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);
    std::signal(SIGPIPE, SIG_IGN);

    ensure_storage_dir("backup_storage");

    TcpServer server(port);
    if (!server.start()) {
        return 1;
    }

    std::cout << "Server listening on port " << port << std::endl;

    while (g_running.load()) {
        std::string client_ip;
        uint16_t client_port = 0;
        std::cout << "Waiting for client connection..." << std::endl;

        int client_sock = server.accept_client(client_ip, client_port);
        if (client_sock == -1) {
            if (!g_running.load()) break;
            continue;
        }

        std::cout << "Client connected from " << client_ip << ":" << client_port << std::endl;

        std::thread([client_sock]() {
            ClientHandler handler(client_sock, "backup_storage");
            handler.run();
        }).detach();
    }

    std::cout << "Server shutting down" << std::endl;
    return 0;
}
