#include "server_logic.h"

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <sys/stat.h>

namespace fs = std::filesystem;

namespace {
constexpr size_t kIoBufferSize = 4096;

const char* kDefaultStorageDir = "backup_storage";

bool send_header(int sock, uint8_t type, uint64_t data_size, uint16_t name_size) {
    FileHeader header{};
    header.type = type;
    header.data_size = htobe64(data_size);
    header.name_size = htobe16(name_size);
    return send_all(sock, &header, sizeof(header)) == 0;
}

bool is_safe_name(const std::string& name) {
    if (name.empty()) return false;
    if (name.find('/') != std::string::npos) return false;
    if (name.find('\\') != std::string::npos) return false;
    if (name == "." || name == "..") return false;
    if (name.find("..") != std::string::npos) return false;
    return true;
}
}

void ensure_storage_dir(const std::string& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        std::cerr << "Failed to create storage dir '" << dir << "': " << ec.message() << std::endl;
    }
}

ClientHandler::ClientHandler(int client_sock, std::string storage_dir)
    : sock_(client_sock), storage_dir_(std::move(storage_dir)) {}

ClientHandler::~ClientHandler() {
    if (sock_ != -1) {
        close(sock_);
    }
}

std::string ClientHandler::safe_path(const std::string& filename) const {
    return storage_dir_ + "/" + filename;
}

bool ClientHandler::read_filename(uint16_t name_len, std::string& out_name) {
    if (name_len == 0) {
        out_name.clear();
        return true;
    }
    std::vector<char> buf(name_len);
    if (recv_all(sock_, buf.data(), name_len) != 0) {
        return false;
    }
    out_name.assign(buf.data(), name_len);
    return is_safe_name(out_name);
}

bool ClientHandler::send_error(uint8_t error_type) {
    return send_header(sock_, error_type, 0, 0);
}

bool ClientHandler::handle_upload(const FileHeader& header) {
    uint64_t data_size = be64toh(header.data_size);
    uint16_t name_len = be16toh(header.name_size);

    std::string name;
    if (!read_filename(name_len, name)) {
        std::cerr << "Upload: invalid filename" << std::endl;
        return false;
    }

    std::string path = safe_path(name);
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Upload: failed to open " << path << " for writing: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    std::vector<char> buffer(kIoBufferSize);
    uint64_t received = 0;
    while (received < data_size) {
        uint64_t remaining = data_size - received;
        size_t to_read = remaining < buffer.size() ? static_cast<size_t>(remaining) : buffer.size();
        ssize_t bytes_in = recv(sock_, buffer.data(), to_read, 0);
        if (bytes_in <= 0) {
            std::cerr << "Upload: connection lost mid-transfer" << std::endl;
            return false;
        }
        out.write(buffer.data(), bytes_in);
        received += static_cast<uint64_t>(bytes_in);
    }

    out.close();
    std::cout << "Received file: " << name << " (" << data_size << " bytes)" << std::endl;
    return true;
}

bool ClientHandler::handle_list() {
    std::ostringstream listing;
    std::error_code ec;

    for (const auto& entry : fs::directory_iterator(storage_dir_, ec)) {
        if (ec) break;
        if (entry.is_regular_file()) {
            listing << entry.path().filename().string() << '\n';
        }
    }

    if (ec) {
        std::cerr << "List: failed to read storage dir: " << ec.message() << std::endl;
        return false;
    }

    std::string list_str = listing.str();
    if (!send_header(sock_, REQ_LIST_FILES, list_str.size(), 0)) {
        return false;
    }
    if (!list_str.empty() && send_all(sock_, list_str.data(), list_str.size()) != 0) {
        return false;
    }
    std::cout << "Sent file list to client (" << list_str.size() << " bytes)" << std::endl;
    return true;
}

bool ClientHandler::handle_download(const FileHeader& header) {
    uint16_t name_len = be16toh(header.name_size);
    std::string name;
    if (!read_filename(name_len, name)) {
        std::cerr << "Download: invalid filename" << std::endl;
        return false;
    }

    std::string path = safe_path(name);
    struct stat st{};
    if (stat(path.c_str(), &st) == -1) {
        std::cerr << "Download: file not found: " << path << std::endl;
        send_header(sock_, REQ_ERROR_NOT_FOUND, 0, 0);
        return true;
    }

    if (send_file(sock_, path.c_str(), REQ_DOWNLOAD) != 0) {
        std::cerr << "Download: send_file failed for " << path << std::endl;
        return false;
    }
    std::cout << "Sent file: " << name << " (" << st.st_size << " bytes)" << std::endl;
    return true;
}

bool ClientHandler::handle_delete(const FileHeader& header) {
    uint16_t name_len = be16toh(header.name_size);
    std::string name;
    if (!read_filename(name_len, name)) {
        std::cerr << "Delete: invalid filename" << std::endl;
        return false;
    }

    std::string path = safe_path(name);
    std::error_code ec;
    bool removed = fs::remove(path, ec);
    if (ec || !removed) {
        std::cerr << "Delete: failed for " << path
                  << " (" << (ec ? ec.message() : std::string{"not found"}) << ")" << std::endl;
        return send_header(sock_, REQ_ERROR_NOT_FOUND, 0, 0);
    }

    std::cout << "Deleted file: " << name << std::endl;
    return send_header(sock_, REQ_DELETE, 0, 0);
}

void ClientHandler::run() {
    FileHeader header{};
    while (true) {
        if (recv_all(sock_, &header, sizeof(header)) != 0) {
            break;
        }

        bool ok = false;
        switch (header.type) {
            case REQ_UPLOAD:     ok = handle_upload(header); break;
            case REQ_LIST_FILES: ok = handle_list(); break;
            case REQ_DOWNLOAD:   ok = handle_download(header); break;
            case REQ_DELETE:     ok = handle_delete(header); break;
            default:
                std::cerr << "Unknown request type: " << static_cast<int>(header.type) << std::endl;
                break;
        }
        if (!ok) break;
    }
    std::cout << "Client connection closed" << std::endl;
}

extern "C" void handle_client(int client_sock) {
    ensure_storage_dir(kDefaultStorageDir);
    ClientHandler handler(client_sock, kDefaultStorageDir);
    handler.run();
}

extern "C" int receive_and_save_file(int client_sock, FileHeader* header) {
    if (!header) return -1;
    uint64_t data_size = be64toh(header->data_size);
    uint16_t name_len = be16toh(header->name_size);

    std::vector<char> name_buf(name_len + 1);
    if (recv_all(client_sock, name_buf.data(), name_len) != 0) {
        return -1;
    }
    name_buf[name_len] = '\0';

    FILE* fp = std::fopen(name_buf.data(), "wb");
    if (!fp) {
        std::perror("File open failed");
        return -1;
    }

    std::vector<char> buffer(kIoBufferSize);
    uint64_t total_received = 0;
    while (total_received < data_size) {
        uint64_t remaining = data_size - total_received;
        size_t to_read = remaining < buffer.size() ? static_cast<size_t>(remaining) : buffer.size();
        ssize_t bytes_in = recv(client_sock, buffer.data(), to_read, 0);
        if (bytes_in <= 0) {
            std::fprintf(stderr, "Connection lost during file transfer\n");
            break;
        }
        std::fwrite(buffer.data(), 1, static_cast<size_t>(bytes_in), fp);
        total_received += static_cast<uint64_t>(bytes_in);
    }
    std::fclose(fp);

    if (total_received == data_size) {
        std::printf("Received file: %s (%llu bytes)\n", name_buf.data(),
                    static_cast<unsigned long long>(data_size));
        return 0;
    }
    return -1;
}

extern "C" int send_file_list(int client_sock) {
    std::ostringstream listing;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(".", ec)) {
        if (ec) break;
        if (entry.is_regular_file()) {
            listing << entry.path().filename().string() << '\n';
        }
    }
    if (ec) return -1;

    std::string list_str = listing.str();
    if (!send_header(client_sock, REQ_LIST_FILES, list_str.size(), 0)) return -1;
    if (!list_str.empty() && send_all(client_sock, list_str.data(), list_str.size()) != 0) return -1;
    return 0;
}

extern "C" int handle_download_request(int client_sock, FileHeader* header) {
    if (!header) return -1;
    uint16_t name_len = be16toh(header->name_size);
    std::vector<char> name_buf(name_len + 1);
    if (recv_all(client_sock, name_buf.data(), name_len) != 0) {
        return -1;
    }
    name_buf[name_len] = '\0';

    struct stat st{};
    if (stat(name_buf.data(), &st) == -1) {
        std::perror("File not found");
        return -1;
    }
    if (send_file(client_sock, name_buf.data(), REQ_DOWNLOAD) != 0) {
        return -1;
    }
    return 0;
}
