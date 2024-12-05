#ifndef __NAMING_SERVER_H
#define __NAMING_SERVER_H


#include "../common/common.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <ifaddrs.h>
// #include <pthread.h>
// #include <sys/types.h>
// #include <errno.h>



#define MAX_SS 10
#define CHARS 256
#define CACHE_CAPACITY 5

// We will make an array of this struct to keep a track of ip and port of ss
typedef struct storage_server_ip_port{
    char ip_address[INET_ADDRSTRLEN];
    int port;
    int alive_checker_port; // specifically for Alive Checker
    int sockfd;
    int backed_up;
    int backup_ss_1;
    int backup_ss_2;
}
storage_server_ip_port;


int initialize_naming_server(char *ip_buffer, int *port);
void *handle_client(void *arg);
void *handle_storage_server(void *arg);
void *accept_client_connections(void *arg) ;
void *accept_storage_connections(void *arg);
void* alive_checker(void* arg);


// Trie part
typedef struct trieNode{
    char value;
    struct trieNode** children;
    int ss_index;  // Will be -1 if not linked to any ss
}
trieNode;

trieNode* createTrieNode(char c);
int add_to_trie(char* file_path, int index);
int get_storage_server_index(char* file_path);
int get_storage_server_index(char* file_path);
void print_trie(char* buffer, int size, trieNode* curr);
int delete_file_path(char* file_path);
int createNewFile(char* filePath, char* filePath2);
int handle_copy(char* dest, char* src);
void copy_to_backup_server(trieNode* curr, char* buffer, int i, int srcIdx, int destIdx);

int search_trie(char* buffer, int size, char* file_path, trieNode *curr, int sockfd);
void collect_trie_data(char *buffer, int size, trieNode *curr, char *output, int *output_size);
void send_trie_optimized(trieNode *curr, int sockfd);


extern trieNode* root;
extern storage_server_ip_port** ss_infos;
extern int num_ss;


// LRU
typedef struct CacheNode {
    char key[50];
    int value;
    struct CacheNode* prev;
    struct CacheNode* next;
} CacheNode;

typedef struct {
    CacheNode* head;
    CacheNode* tail;
    CacheNode* hashmap[CACHE_CAPACITY];
    int size;
} LRUCache;

int hash(const char* key);
CacheNode* createNode(const char* key, int value);
LRUCache* createCache();
void moveToHead(LRUCache* cache, CacheNode* node);
void removeLRU(LRUCache* cache);
void put(LRUCache* cache, const char* key, int value);
int get(LRUCache* cache, const char* key);
void freeCache(LRUCache* cache);
void removeFromCache(LRUCache* cache, const char* key);
void createBackup(int srcIdx, int destIdx);
void handleBackup();
void handle_dirs_rec(char* buffer, int depth, trieNode* curr, int srcIdx, int destIdx, char* src, char* dest);
void handle_dir(char* dest, char* src, int destIdx, int srcIdx);

extern LRUCache* cache;


#define LOG(sockfd, result, fmt, args...)                                       \
  do {                                                                         \
    char ip[INET6_ADDRSTRLEN] = "unknown";                                     \
    int port = -1;                                                             \
    struct sockaddr_storage peer_addr;                                         \
    socklen_t addr_len = sizeof(peer_addr);                                    \
                                                                               \
    if (getpeername(sockfd, (struct sockaddr *)&peer_addr, &addr_len) == 0) {  \
      if (peer_addr.ss_family == AF_INET) {                                    \
        struct sockaddr_in *addr_in = (struct sockaddr_in *)&peer_addr;        \
        inet_ntop(AF_INET, &addr_in->sin_addr, ip, sizeof(ip));                \
        port = ntohs(addr_in->sin_port);                                       \
      } else if (peer_addr.ss_family == AF_INET6) {                            \
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&peer_addr;     \
        inet_ntop(AF_INET6, &addr_in6->sin6_addr, ip, sizeof(ip));             \
        port = ntohs(addr_in6->sin6_port);                                     \
      }                                                                        \
    }                                                                          \
                                                                               \
    FILE *ptr = fopen("logfile.log", "a");                                     \
    if (ptr) {                                                                 \
      time_t currentTime;                                                      \
      time(&currentTime);                                                      \
      char timestamp[20];                                                      \
      strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S",              \
               localtime(&currentTime));                                       \
                                                                               \
      if (result == -2) {                                                      \
        /* Ignore the result and log only the message */                       \
        fprintf(ptr, "[%s] [%s:%d] %s:%d:%s() - " fmt "\n",                    \
                timestamp, ip, port, __FILE__, __LINE__, __func__, ##args);    \
      } else {                                                                 \
        const char *status = (result == -1) ? "FAILED" : "PASSED";             \
        fprintf(ptr, "[%s] [%s:%d] %s:%d:%s() - %s - " fmt "\n",               \
                timestamp, ip, port, __FILE__, __LINE__, __func__, status, ##args); \
      }                                                                        \
      fclose(ptr);                                                             \
    } else {                                                                   \
      fprintf(stderr, "Failed to open log file\n");                            \
    }                                                                          \
  } while (0)



#endif