#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ensures all requested bytes are sent to the socket.
 * @param sock The socket file descriptor.
 * @param buffer The buffer containing data to send.
 * @param len The number of bytes to send.
 * @return 0 on success, -1 on failure.
 */
int send_all(int sock, const void *buffer, size_t len);

/**
 * Ensures all requested bytes are received from the socket.
 * @param sock The socket file descriptor.
 * @param buffer The buffer to store received data.
 * @param len The number of bytes to receive.
 * @return 0 on success, -1 on failure.
 */
int recv_all(int sock, void *buffer, size_t len);

/**
 * Sends a file over a socket following the protocol:
 * 1. FileHeader (with type, data_size, name_size)
 * 2. Filename
 * 3. File content
 *
 * @param sock The socket to send data to.
 * @param filepath The path to the file to be sent.
 * @param type The type of the request (e.g., upload/download).
 * @return 0 on success, -1 on failure.
 */
int send_file(int sock, const char *filepath, uint8_t type);

#ifdef __cplusplus
}
#endif

#endif // FILE_UTILS_H