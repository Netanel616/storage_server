#ifndef SHARED_PROTOCOL_H
#define SHARED_PROTOCOL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htobe16(x) OSSwapHostToBigInt16(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#else
#include <endian.h>
#endif

#define SERVER_PORT 54321
#define SERVER_IP "127.0.0.1"

// Request Types
#define REQ_UPLOAD 1
#define REQ_LIST_FILES 2
#define REQ_DOWNLOAD 3

#pragma pack(push, 1)
typedef struct
{
    uint64_t data_size; // the size of the data
    uint16_t name_size; // the size of the name
    uint8_t type; // will use for the request type
}FileHeader;
#pragma pack(pop)

#endif //SHARED_PROTOCOL_H