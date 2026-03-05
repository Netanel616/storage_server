#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <limits>
#include <algorithm> // for std::min
#include "../shared/protocol.h"
#include "../shared/file_utils.h"

class BackupClient {
public:
    BackupClient(const std::string& server_ip, int port) : server_ip_(server_ip), port_(port), sock_(-1) {}

    ~BackupClient() {
        if (sock_ != -1) {
            close(sock_);
        }
    }

    bool connect_to_server() {
        sock_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_ == -1) {
            std::cerr << "Socket creation failed" << std::endl;
            return false;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address/ Address not supported" << std::endl;
            return false;
        }

        if (connect(sock_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed" << std::endl;
            return false;
        }

        std::cout << "Connected to server at " << server_ip_ << ":" << port_ << std::endl;
        return true;
    }

    void run() {
        int choice;
        while (true) {
            print_menu();
            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            switch (choice) {
                case 1:
                    handle_upload();
                    break;
                case 2:
                    handle_download();
                    break;
                case 3:
                    handle_list_files();
                    break;
                case 4:
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

    void print_menu() {
        std::cout << "\n--- Mini Backup Client ---\n";
        std::cout << "1. Upload File\n";
        std::cout << "2. Download File\n";
        std::cout << "3. List Files\n";
        std::cout << "4. Exit\n";
        std::cout << "Select an option: ";
    }

    void handle_upload() {
        std::string filepath;
        std::cout << "Enter file path to upload: ";
        std::cin >> filepath;

        if (send_file(sock_, filepath.c_str(), REQ_UPLOAD) == 0) {
            std::cout << "File uploaded successfully." << std::endl;
        } else {
            std::cerr << "Failed to upload file." << std::endl;
        }
    }

    void handle_download() {
        std::string filename;
        std::cout << "Enter filename to download: ";
        std::cin >> filename;

        uint16_t name_len = static_cast<uint16_t>(filename.length());
        FileHeader req;
        req.type = REQ_DOWNLOAD;
        req.data_size = 0;
        req.name_size = htobe16(name_len);

        if (send_all(sock_, &req, sizeof(req)) != 0) {
            std::cerr << "Failed to send download request" << std::endl;
            return;
        }

        if (send_all(sock_, filename.c_str(), name_len) != 0) {
            std::cerr << "Failed to send filename" << std::endl;
            return;
        }

        FileHeader resp;
        if (recv_all(sock_, &resp, sizeof(resp)) != 0) {
            std::cerr << "Failed to receive download response" << std::endl;
            return;
        }

        if (resp.type != REQ_DOWNLOAD) {
            std::cerr << "Unexpected response type: " << (int)resp.type << " (Server might not have the file)" << std::endl;
            return;
        }

        uint64_t file_size = be64toh(resp.data_size);
        uint16_t resp_name_len = be16toh(resp.name_size);

        std::vector<char> name_buffer(resp_name_len + 1);
        if (recv_all(sock_, name_buffer.data(), resp_name_len) != 0) {
            std::cerr << "Failed to receive filename" << std::endl;
            return;
        }
        name_buffer[resp_name_len] = '\0';
        std::string resp_filename(name_buffer.data());

        std::cout << "Downloading " << resp_filename << " (" << file_size << " bytes)..." << std::endl;

        std::ofstream outfile(resp_filename, std::ios::binary);
        if (!outfile.is_open()) {
            std::cerr << "Failed to open file for writing: " << resp_filename << std::endl;
            return;
        }

        std::vector<char> data_buffer(4096);
        uint64_t total_received = 0;
        while (total_received < file_size) {
            uint64_t to_read = std::min(static_cast<uint64_t>(data_buffer.size()), file_size - total_received);
            ssize_t bytes_in = recv(sock_, data_buffer.data(), to_read, 0);
            if (bytes_in <= 0) {
                std::cerr << "Connection lost during download." << std::endl;
                break;
            }
            outfile.write(data_buffer.data(), bytes_in);
            total_received += bytes_in;
        }

        if (total_received == file_size) {
            std::cout << "Download complete." << std::endl;
        } else {
            std::cerr << "Download incomplete." << std::endl;
        }
    }

    void handle_list_files() {
        FileHeader req;
        req.type = REQ_LIST_FILES;
        req.data_size = 0;
        req.name_size = 0;

        if (send_all(sock_, &req, sizeof(req)) != 0) {
            std::cerr << "Failed to send list request" << std::endl;
            return;
        }

        FileHeader resp;
        if (recv_all(sock_, &resp, sizeof(resp)) != 0) {
            std::cerr << "Failed to receive list response" << std::endl;
            return;
        }

        if (resp.type != REQ_LIST_FILES) {
            std::cerr << "Unexpected response type: " << (int)resp.type << std::endl;
            return;
        }

        uint64_t list_len = be64toh(resp.data_size);
        std::vector<char> list_buffer(list_len + 1);
        if (recv_all(sock_, list_buffer.data(), list_len) != 0) {
            std::cerr << "Failed to receive file list" << std::endl;
            return;
        }
        list_buffer[list_len] = '\0';

        std::cout << "\n--- Server Files ---\n";
        std::cout << list_buffer.data();
        std::cout << "--------------------\n";
    }
};

int main() {
    BackupClient client(SERVER_IP, SERVER_PORT);
    if (client.connect_to_server()) {
        client.run();
    }
    return 0;
}
