#ifndef __COMMON_HEADER__
#define __COMMON_HEADER__

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/time.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <sys/types.h>
#include <errno.h>
#include<unistd.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <stdarg.h>

// Client
#define MAX_TOKENS 5
#define MAX_STRING_SIZE 256

// Storage server
#define BUFFER_SIZE 1000000
#define MAX_PATHS 5000
#define MAX_PATH_LENGTH 1024

// ENUM to define error codes
typedef enum error_code
{
    SUCCESS,
    INVALID_PATH,
    NO_READ_PERMISSION,
    NO_WRITE_PERMISSION,
    NO_DELETE_PERMISSION,
    MAIN_SERVER_DOWN,
    ALL_BACKUPS_DOWN,
}
error_code;

// operation enum
typedef enum operation{
    READ,
    WRITE,
    GETINFO,
    STREAM,
    LIST,
    DELETE,
    CREATE,
    COPY
} operation;

// struct for client to NM communication
typedef struct client_to_NM{
    char file_path[MAX_STRING_SIZE];
    char file_path_2[MAX_STRING_SIZE];
    operation oper;
    int is_async;
} client_to_NM;

// struct for NM to client communication
typedef struct NM_to_client{
    char IP_SS[INET_ADDRSTRLEN];
    int port_SS;
    int ack;
    error_code e_code;
} NM_to_client;

// struct for communication between NM and SS
typedef struct SS_to_NM_struct {
    char buffer[BUFFER_SIZE];
    size_t buffer_size;
    int stop;
    int is_directory;  // New flag to indicate directory transfers
} SS_to_NM_struct;

typedef struct StorageServerInfo {
    char ip[INET_ADDRSTRLEN];
    int nm_connection_port;
    int client_connection_port;
    int alive_connection_port; // specific to alive checker
    int path_count;
    char accessible_paths[MAX_PATHS][MAX_PATH_LENGTH];
}StorageServerInfo;

typedef struct {
    int client_sockfd;
    struct sockaddr_in client_addr;
} client_info_t;

typedef struct {
    int storage_sockfd;
    struct sockaddr_in storage_addr;
} storage_info_t;

#endif