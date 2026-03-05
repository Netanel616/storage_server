#include "network.h"
#include "server_logic.h"
#include "../shared/protocol.h"

int main(void)
{
    int sock = setup_server(SERVER_PORT);
    if (sock == -1)
    {
        return 1;
    }

    printf("Server listening on port %d\n", SERVER_PORT);

    while(1)
    {
        struct sockaddr_in client_addr;
        printf("Waiting for client connection...\n");
        
        int client_sock = accept_new_connection(sock, &client_addr);
        if (client_sock == -1)
        {
            continue; 
        }

        printf("Client connected from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        handle_client(client_sock);
    }

    close(sock);
    return 0;
}
