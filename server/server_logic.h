#ifndef SERVER_LOGIC_H
#define SERVER_LOGIC_H

#include "../shared/protocol.h"
#include "../shared/file_utils.h"

//handle all communication with one client
void handle_client(int client_sock);

// helper to open and save a file
int receive_and_save_file(int client_sock, FileHeader *header);

// helper to send the list of files in the current directory
int send_file_list(int client_sock);

// helper to handle a download request from a client
int handle_download_request(int client_sock, FileHeader *header);

#endif