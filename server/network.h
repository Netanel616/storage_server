#ifndef NETWORK_H
#define NETWORK_H

#include "../shared/protocol.h"

/**
 * מקימה socket, מגדירה REUSEADDR, מבצעת bind ומתחילה להאזין (listen).
 * @param port הפורט עליו השרת יאזין.
 * @return ה-socket file descriptor של השרת, או -1 במקרה של שגיאה.
 */
int setup_server(int port);

/**
 * ממתינה לחיבור חדש (accept).
 * @param server_sock ה-socket המאזין של השרת.
 * @param client_addr מצביע למבנה שבו יישמרו פרטי הלקוח.
 * @return ה-socket file descriptor של הלקוח שהתחבר.
 */
int accept_new_connection(int server_sock, struct sockaddr_in *client_addr);

#endif // NETWORK_H