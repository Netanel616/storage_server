#include "file_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h> // for basename

int send_all(int sock, const void *buffer, size_t len) {
    size_t total_sent = 0;
    const char *ptr = (const char *)buffer;
    while (total_sent < len) {
        ssize_t bytes_out = send(sock, ptr + total_sent, len - total_sent, 0);
        if (bytes_out <= 0) return -1;
        total_sent += bytes_out;
    }
    return 0;
}

int recv_all(int sock, void *buffer, size_t len) {
    size_t total_received = 0;
    char *ptr = (char *)buffer;
    while (total_received < len) {
        ssize_t bytes_in = recv(sock, ptr + total_received, len - total_received, 0);
        if (bytes_in <= 0) return -1;
        total_received += bytes_in;
    }
    return 0;
}

int send_file(int sock, const char *filepath, uint8_t type) {
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        perror("Failed to open file");
        return -1;
    }

    // Get file size
    struct stat st;
    if (stat(filepath, &st) == -1) {
        perror("Failed to get file stats");
        fclose(fp);
        return -1;
    }
    uint64_t file_size = (uint64_t)st.st_size;

    // Get filename from path
    // Note: basename might modify the string, so we use a copy if needed,
    // but here we just need the pointer. However, standard basename
    // behavior varies. We'll assume POSIX basename which might modify.
    // To be safe, let's duplicate the path for basename.
    char *path_copy = strdup(filepath);
    if (!path_copy) {
        fclose(fp);
        return -1;
    }
    char *filename = basename(path_copy);
    uint16_t name_len = (uint16_t)strlen(filename);

    // Prepare header
    FileHeader header;
    header.type = type;
    header.data_size = htobe64(file_size);
    header.name_size = htobe16(name_len);

    // 1. Send Header
    if (send_all(sock, &header, sizeof(header)) != 0) {
        perror("Failed to send header");
        free(path_copy);
        fclose(fp);
        return -1;
    }

    // 2. Send Filename
    if (send_all(sock, filename, name_len) != 0) {
        perror("Failed to send filename");
        free(path_copy);
        fclose(fp);
        return -1;
    }

    free(path_copy); // Done with filename

    // 3. Send File Content
    char buffer[4096];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        if (send_all(sock, buffer, bytes_read) != 0) {
            perror("Failed to send file content");
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return 0;
}
