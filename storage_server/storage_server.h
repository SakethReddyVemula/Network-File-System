#ifndef __STORAGE_SERVER_H__
#define __STORAGE_SERVER_H__

#include "../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>

#define MAX_FILES 100

typedef struct {
    char filepath[PATH_MAX];
    pthread_rwlock_t lock;
    int is_initialized;
} file_lock_t;

typedef struct {
    file_lock_t locks[MAX_FILES];
    int count;
    pthread_mutex_t lock_array_mutex;
} file_lock_manager_t;

extern file_lock_manager_t lock_manager;

typedef struct {
    int client_socket;
    char path[256];
    char buffer[BUFFER_SIZE];
} ThreadArgs;

extern int nm_sockfd, client_sockfd;
extern const char *storage_directory;

void error(const char *msg);
void get_ip_address(char *ip_buffer, size_t buffer_size);
void initialize_storage_server(const char *nm_ip, int nm_port, int predefined_client_connection_port);

void recursive_path_search(const char *directory, char ***paths, int *path_count);
char **get_accessible_paths(int *path_count);

void *accept_client_connections(void *arg);
void *handle_client(void *arg);

void *accept_nm_connections(void *arg);

// alive checker
int get_port(int sockfd);
void* alive_checker_connection(void *arg);

// locks
void init_lock_manager();
pthread_rwlock_t* get_file_lock(const char* filepath);
void cleanup_lock_manager();

void send_error_to_client(int client_socket, const char* error_msg);
void nm_read_file(int nm_socket, const char *path);

int is_directory(const char *path);

#endif
