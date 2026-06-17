#include "../shared/protocol.h"
#include "../shared/file_utils.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

class BackupClient {
public:
    BackupClient(std::string server_ip, int port)
        : server_ip_(std::move(server_ip)), port_(port), sock_(-1) {}

    ~BackupClient() {
        disconnect();
    }

    BackupClient(const BackupClient&) = delete;
    BackupClient& operator=(const BackupClient&) = delete;

    bool connect_to_server() {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ == -1) {
            std::cerr << "Socket creation failed: " << std::strerror(errno) << std::endl;
            return false;
        }

        struct sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address: " << server_ip_ << std::endl;
            disconnect();
            return false;
        }

        if (connect(sock_, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            std::cerr << "Connection to " << server_ip_ << ":" << port_
                      << " failed: " << std::strerror(errno) << std::endl;
            disconnect();
            return false;
        }

        std::cout << "Connected to server at " << server_ip_ << ":" << port_ << std::endl;
        return true;
    }

    void run() {
        while (true) {
            print_menu();
            int choice = 0;
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Please enter a number." << std::endl;
                continue;
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            switch (choice) {
                case 1: handle_upload();     break;
                case 2: handle_download();   break;
                case 3: handle_list_files(); break;
                case 4: handle_delete();     break;
                case 5:
                    std::cout << "Exiting..." << std::endl;
                    return;
                default:
                    std::cout << "Invalid option." << std::endl;
            }
        }
    }

private:
    std::string server_ip_;
    int port_;
    int sock_;

    void disconnect() {
        if (sock_ != -1) {
            close(sock_);
            sock_ = -1;
        }
    }

    void print_menu() {
        std::cout << "\n--- Mini Backup Client ---\n"
                  << "1. Upload File\n"
                  << "2. Download File\n"
                  << "3. List Files\n"
                  << "4. Delete File\n"
                  << "5. Exit\n"
                  << "Select an option: ";
    }

    static std::string read_line(const std::string& prompt) {
        std::cout << prompt;
        std::string s;
        std::getline(std::cin, s);
        return s;
    }

    bool send_simple_request(uint8_t type, const std::string& filename) {
        uint16_t name_len = static_cast<uint16_t>(filename.size());
        FileHeader req{};
        req.type = type;
        req.data_size = 0;
        req.name_size = htobe16(name_len);

        if (send_all(sock_, &req, sizeof(req)) != 0) {
            std::cerr << "Failed to send request header" << std::endl;
            return false;
        }
        if (name_len > 0 && send_all(sock_, filename.data(), name_len) != 0) {
            std::cerr << "Failed to send filename" << std::endl;
            return false;
        }
        return true;
    }

    void handle_upload() {
        std::string filepath = read_line("Enter file path to upload: ");
        if (filepath.empty()) {
            std::cout << "Cancelled." << std::endl;
            return;
        }

        std::ifstream check(filepath, std::ios::binary);
        if (!check.is_open()) {
            std::cerr << "File not found locally: " << filepath << std::endl;
            return;
        }
        check.close();

        if (send_file(sock_, filepath.c_str(), REQ_UPLOAD) == 0) {
            std::cout << "File uploaded successfully." << std::endl;
        } else {
            std::cerr << "Failed to upload file." << std::endl;
        }
    }

    void handle_download() {
        std::string filename = read_line("Enter filename to download: ");
        if (filename.empty()) {
            std::cout << "Cancelled." << std::endl;
            return;
        }

        if (!send_simple_request(REQ_DOWNLOAD, filename)) return;

        FileHeader resp{};
        if (recv_all(sock_, &resp, sizeof(resp)) != 0) {
            std::cerr << "Failed to receive download response" << std::endl;
            return;
        }

        if (resp.type == REQ_ERROR_NOT_FOUND) {
            std::cerr << "Server: file not found." << std::endl;
            return;
        }
        if (resp.type != REQ_DOWNLOAD) {
            std::cerr << "Unexpected response type: " << static_cast<int>(resp.type) << std::endl;
            return;
        }

        uint64_t file_size = be64toh(resp.data_size);
        uint16_t resp_name_len = be16toh(resp.name_size);

        std::vector<char> name_buffer(resp_name_len);
        if (resp_name_len > 0 && recv_all(sock_, name_buffer.data(), resp_name_len) != 0) {
            std::cerr << "Failed to receive filename" << std::endl;
            return;
        }
        std::string resp_filename(name_buffer.data(), resp_name_len);

        std::cout << "Downloading " << resp_filename << " (" << file_size << " bytes)..." << std::endl;

        std::ofstream outfile(resp_filename, std::ios::binary);
        if (!outfile.is_open()) {
            std::cerr << "Failed to open file for writing: " << resp_filename << std::endl;
            drain_bytes(file_size);
            return;
        }

        std::vector<char> data_buffer(4096);
        uint64_t total_received = 0;
        while (total_received < file_size) {
            uint64_t remaining = file_size - total_received;
            size_t to_read = static_cast<size_t>(std::min<uint64_t>(data_buffer.size(), remaining));
            ssize_t bytes_in = recv(sock_, data_buffer.data(), to_read, 0);
            if (bytes_in <= 0) {
                std::cerr << "Connection lost during download." << std::endl;
                return;
            }
            outfile.write(data_buffer.data(), bytes_in);
            total_received += static_cast<uint64_t>(bytes_in);
        }

        std::cout << "Download complete: " << resp_filename << std::endl;
    }

    void handle_list_files() {
        if (!send_simple_request(REQ_LIST_FILES, "")) return;

        FileHeader resp{};
        if (recv_all(sock_, &resp, sizeof(resp)) != 0) {
            std::cerr << "Failed to receive list response" << std::endl;
            return;
        }

        if (resp.type != REQ_LIST_FILES) {
            std::cerr << "Unexpected response type: " << static_cast<int>(resp.type) << std::endl;
            return;
        }

        uint64_t list_len = be64toh(resp.data_size);
        std::string list_buffer(list_len, '\0');
        if (list_len > 0 && recv_all(sock_, (void*)list_buffer.data(), list_len) != 0) {
            std::cerr << "Failed to receive file list" << std::endl;
            return;
        }

        std::cout << "\n--- Server Files ---\n";
        if (list_buffer.empty()) {
            std::cout << "(no files)\n";
        } else {
            std::cout << list_buffer;
        }
        std::cout << "--------------------\n";
    }

    void handle_delete() {
        std::string filename = read_line("Enter filename to delete: ");
        if (filename.empty()) {
            std::cout << "Cancelled." << std::endl;
            return;
        }

        if (!send_simple_request(REQ_DELETE, filename)) return;

        FileHeader resp{};
        if (recv_all(sock_, &resp, sizeof(resp)) != 0) {
            std::cerr << "Failed to receive delete response" << std::endl;
            return;
        }

        if (resp.type == REQ_DELETE) {
            std::cout << "File deleted: " << filename << std::endl;
        } else if (resp.type == REQ_ERROR_NOT_FOUND) {
            std::cerr << "Server: file not found." << std::endl;
        } else {
            std::cerr << "Unexpected response type: " << static_cast<int>(resp.type) << std::endl;
        }
    }

    void drain_bytes(uint64_t count) {
        std::vector<char> buf(4096);
        while (count > 0) {
            size_t to_read = static_cast<size_t>(std::min<uint64_t>(buf.size(), count));
            ssize_t in = recv(sock_, buf.data(), to_read, 0);
            if (in <= 0) return;
            count -= static_cast<uint64_t>(in);
        }
    }
};

int main(int argc, char* argv[]) {
    std::string host = SERVER_IP;
    int port = SERVER_PORT;

    if (argc >= 2) host = argv[1];
    if (argc >= 3) {
        try {
            port = std::stoi(argv[2]);
        } catch (const std::exception& e) {
            std::cerr << "Invalid port '" << argv[2] << "': " << e.what() << std::endl;
            return 1;
        }
    }

    BackupClient client(host, port);
    if (!client.connect_to_server()) {
        return 1;
    }
    client.run();
    return 0;
}
