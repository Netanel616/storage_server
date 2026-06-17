#ifndef SERVER_LOGIC_H
#define SERVER_LOGIC_H

#include "../shared/protocol.h"
#include "../shared/file_utils.h"

#ifdef __cplusplus
#include <string>

class ClientHandler {
public:
    ClientHandler(int client_sock, std::string storage_dir);
    ~ClientHandler();

    ClientHandler(const ClientHandler&) = delete;
    ClientHandler& operator=(const ClientHandler&) = delete;

    void run();

private:
    int sock_;
    std::string storage_dir_;

    bool handle_upload(const FileHeader& header);
    bool handle_list();
    bool handle_download(const FileHeader& header);
    bool handle_delete(const FileHeader& header);

    bool send_error(uint8_t error_type);
    std::string safe_path(const std::string& filename) const;
    bool read_filename(uint16_t name_len, std::string& out_name);
};

void ensure_storage_dir(const std::string& dir);

extern "C" {
#endif

void handle_client(int client_sock);
int receive_and_save_file(int client_sock, FileHeader* header);
int send_file_list(int client_sock);
int handle_download_request(int client_sock, FileHeader* header);

#ifdef __cplusplus
}
#endif

#endif
