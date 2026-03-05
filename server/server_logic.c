#include "server_logic.h"
#include "../shared/file_utils.h"
#include <dirent.h>
#include <sys/stat.h>

//arrange aall included files in this project and put all the module that more than one file use in the shared file

int receive_and_save_file(int client_sock, FileHeader *header) {
    uint64_t package_size = be64toh(header->data_size);
    uint16_t name_len = be16toh(header->name_size);

    char *name = malloc(name_len + 1);
    if (!name) return -1;

    if (recv_all(client_sock, name, name_len) != 0) {
        free(name);
        return -1;
    }
    name[name_len] = '\0';

    FILE *fp = fopen(name, "wb");
    if (fp == NULL) {
        perror("File open failed");
        free(name);
        return -1;
    }

    uint64_t total_received = 0;
    char data_buffer[4096];

    while (total_received < package_size) {
        uint64_t to_read = package_size - total_received;
        if (to_read > sizeof(data_buffer)) to_read = sizeof(data_buffer);

        ssize_t bytes_in = recv(client_sock, data_buffer, (size_t)to_read, 0);
        if (bytes_in <= 0) {
            fprintf(stderr, "Connection lost during file transfer\n");
            break;
        }

        fwrite(data_buffer, 1, bytes_in, fp);
        total_received += bytes_in;
    }

    fclose(fp);
    if (total_received == package_size) {
        printf("Received file: %s (%llu bytes)\n", name, (unsigned long long)package_size);
    }
    free(name);
    return (total_received == package_size) ? 0 : -1;
}

int send_file_list(int client_sock) {
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (!d) {
        perror("Failed to open directory");
        return -1;
    }

    // Calculate total size of the list string
    size_t total_len = 0;
    // We'll construct a single string with filenames separated by newlines
    // First pass: calculate size
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) { // Regular files only
            total_len += strlen(dir->d_name) + 1; // +1 for newline
        }
    }
    rewinddir(d);

    char *list_buffer = malloc(total_len + 1);
    if (!list_buffer) {
        closedir(d);
        return -1;
    }
    list_buffer[0] = '\0';

    // Second pass: build the string
    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_REG) {
            strcat(list_buffer, dir->d_name);
            strcat(list_buffer, "\n");
        }
    }
    closedir(d);

    // Send header
    FileHeader header;
    header.type = REQ_LIST_FILES;
    header.data_size = htobe64(total_len);
    header.name_size = 0; // No filename for list response

    if (send_all(client_sock, &header, sizeof(header)) != 0) {
        free(list_buffer);
        return -1;
    }

    // Send list content
    if (send_all(client_sock, list_buffer, total_len) != 0) {
        free(list_buffer);
        return -1;
    }

    free(list_buffer);
    printf("Sent file list to client\n");
    return 0;
}

int handle_download_request(int client_sock, FileHeader *header) {
    uint16_t name_len = be16toh(header->name_size);
    char *filename = malloc(name_len + 1);
    if (!filename) return -1;

    if (recv_all(client_sock, filename, name_len) != 0) {
        free(filename);
        return -1;
    }
    filename[name_len] = '\0';

    printf("Client requested download for file: %s\n", filename);

    // Check if file exists
    struct stat st;
    if (stat(filename, &st) == -1) {
        perror("File not found");
        // Ideally send an error response, but for now just close connection or send empty
        free(filename);
        return -1;
    }

    // Send the file back using REQ_DOWNLOAD type so client knows it's the file content
    if (send_file(client_sock, filename, REQ_DOWNLOAD) != 0) {
        fprintf(stderr, "Failed to send file %s\n", filename);
        free(filename);
        return -1;
    }

    printf("File %s sent successfully.\n", filename);
    free(filename);
    return 0;
}

void handle_client(int client_sock) {
    FileHeader header;
    while (1) {
        if (recv_all(client_sock, &header, sizeof(header)) != 0) {
            break;
        }

        if (header.type == REQ_UPLOAD) {
             if (receive_and_save_file(client_sock, &header) != 0) {
                break;
            }
        } else if (header.type == REQ_LIST_FILES) {
            if (send_file_list(client_sock) != 0) {
                break;
            }
        } else if (header.type == REQ_DOWNLOAD) {
            if (handle_download_request(client_sock, &header) != 0) {
                break;
            }
        } else {
            fprintf(stderr, "Unknown request type: %d\n", header.type);
            break;
        }
    }
    close(client_sock);
    printf("Client connection closed\n");
}
