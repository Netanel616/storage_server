#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../shared/protocol.h"

void test_endianness_macros() {
    printf("Testing endianness macros...\n");
    uint64_t val64 = 0x1234567890ABCDEF;
    uint64_t be64 = htobe64(val64);
    assert(be64toh(be64) == val64);

    uint16_t val16 = 0x1234;
    uint16_t be16 = htobe16(val16);
    assert(be16toh(be16) == val16);
    printf("endianness macros passed.\n");
}

void test_file_header_struct() {
    printf("Testing FileHeader struct...\n");
    FileHeader header;
    header.data_size = 100;
    header.name_size = 20;
    header.type = 1;

    assert(sizeof(header) == sizeof(uint64_t) + sizeof(uint16_t) + sizeof(uint8_t));
    printf("FileHeader struct passed.\n");
}

int main() {
    test_endianness_macros();
    test_file_header_struct();
    printf("All protocol tests passed!\n");
    return 0;
}
