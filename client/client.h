#ifndef __CLIENT_HEADER__
#define __CLIENT_HEADER__

#include"../common/common.h"

typedef struct {
    int sock_SS;
    const char* file_path;
    char buffer[BUFFER_SIZE];
} async_write_args;

int connect_to_server(char* ip_address, int port);
int tokenize(char* buffer, char** tokens);
int accept_connection_acknowledgement(int sock);
void print_file_info(const struct stat *file_stat);

#endif