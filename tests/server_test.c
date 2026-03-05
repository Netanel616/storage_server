#include "../shared/protocol.h"

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return 1;
    }
    char *dummy_data = "This is the content of the file. It could be anything!";
    uint64_t data_len = strlen(dummy_data);

    // רשימת קבצים דמיוניים לבדיקה
    char *test_files[] = {"vacation.jpg", "budget.pdf", "notes.txt"};

    for (int i = 0; i < 3; i++) {
        FileHeader header;
        header.type = 1; // Upload
        header.data_size = htobe64(data_len); // גודל קובץ פיקטיבי

        uint16_t name_len = strlen(test_files[i]);
        header.name_size = htobe16(name_len);

        printf("Sending Header for: %s (Length: %u)\n", test_files[i], name_len);

        // 1. שלח את ה-Header
        send(sock, &header, sizeof(FileHeader), 0);

        // 2. שלח את שם הקובץ (בדיוק name_len בתים, בלי \0)
        send(sock, test_files[i], name_len, 0);

        send(sock, dummy_data, data_len, 0);

        sleep(1); // חכה שנייה בין קובץ לקובץ
    }

    close(sock);
    printf("Test finished.\n");
    return 0;
}
