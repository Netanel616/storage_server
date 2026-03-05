# Mini Backup Server

## Overview

This is a simple, cross-platform backup server written in C. It allows clients to connect over TCP, upload files for backup, and request a list of all currently backed-up files. The server is designed to be lightweight and portable, with a focus on a simple, well-defined protocol.

## Features

*   **File Upload**: Clients can send files to the server. The server receives them and saves them in its root execution directory.
*   **File Listing**: Clients can request a complete list of all files currently stored on the server. The server will respond with a newline-separated list of filenames.
*   **Cross-Platform Compatibility**: Uses standard C libraries and platform-specific headers (e.g., for endianness) to work on both Linux and macOS.
*   **Simple Protocol**: Communication is handled via a straightforward binary protocol defined in `shared/protocol.h`.

## Protocol

The communication protocol is based on a simple header structure that precedes any data transfer.

### `FileHeader`

All communications start with this header:

```c
#pragma pack(push, 1)
typedef struct
{
    uint64_t data_size; // The size of the data payload that follows.
    uint16_t name_size; // The size of the filename (if applicable).
    uint8_t type;       // The type of request.
} FileHeader;
#pragma pack(pop)
```

### Request Types

1.  **`REQ_UPLOAD` (Value: 1)**
    *   **Purpose**: To upload a file to the server.
    *   **Structure**: `FileHeader` -> `Filename` -> `File Content`
    *   The client sends a header with the file content size and filename size, followed by the filename, and then the raw file data.

2.  **`REQ_LIST_FILES` (Value: 2)**
    *   **Purpose**: To request a list of all files on the server.
    *   **Client Request**: The client sends a `FileHeader` with `type = REQ_LIST_FILES`. `data_size` and `name_size` can be 0.
    *   **Server Response**: The server replies with a `FileHeader` where `data_size` is the total length of the file list string, followed by the string itself (e.g., "file1.txt\nfile2.jpg\nnotes.pdf\n").

## How to Build and Run

It is recommended to use an out-of-source build to keep the project directory clean.

```sh
# 1. Create and navigate to a build directory
mkdir -p build
cd build

# 2. Generate build files with CMake
cmake ..

# 3. Compile the project
make

# 4. Run the server
./backup_server
```
The server will start listening for connections on port `54321`.

## How to Test

The project includes a suite of unit and integration tests.

```sh
# From the build directory, run:
ctest --verbose
```

---

## To-Do List & Future Improvements

-   [ ] **File Download**: Implement a `REQ_DOWNLOAD` request type to allow clients to retrieve a specific file from the server.
-   [ ] **File Deletion**: Add a `REQ_DELETE` request type for removing files from the backup.
-   [ ] **Dedicated Backup Directory**: Create a dedicated subdirectory (e.g., `backup_storage/`) for storing files, instead of saving them in the server's root directory. This improves organization and security.
-   [ ] **User Authentication**: Implement a login system (`REQ_AUTH`) to ensure only authorized users can upload or access files.
-   [ ] **Concurrency**: Handle multiple clients simultaneously. This could be achieved using:
    -   Multi-threading (a new thread per client).
    -   A thread pool.
    -   I/O multiplexing with `select()`, `poll()`, or `epoll()`.
-   [ ] **Robust Error Handling**: Send specific error response headers back to the client (e.g., `ERR_FILE_NOT_FOUND`, `ERR_ACCESS_DENIED`) instead of just closing the connection.
-   [ ] **Configuration File**: Allow the server port and backup directory to be configured via a file (e.g., `server.conf`) or command-line arguments.
-   [ ] **Client Implementation**: Develop the corresponding client applications (desktop/mobile) that will interact with the server.
