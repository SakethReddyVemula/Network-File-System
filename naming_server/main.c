#include "naming_server.h"


trieNode* root;
storage_server_ip_port** ss_infos;
LRUCache* cache;
int num_ss = 0;

int main() {
    root = createTrieNode('/');
    cache = createCache();
    ss_infos = (storage_server_ip_port**)(malloc(sizeof(storage_server_ip_port) * MAX_SS));
    for (int i = 0; i < MAX_SS; i++){
        ss_infos[i] = (storage_server_ip_port*)(malloc(sizeof(storage_server_ip_port)));
        ss_infos[i]->port = -1;
    }
    char ip_buffer[INET_ADDRSTRLEN] = {0};
    int client_port, storage_port;

    // Initialize naming server for clients
    int client_server_sockfd = initialize_naming_server(ip_buffer, &client_port);
    if (client_server_sockfd == -1) {
        fprintf(stderr, "Failed to initialize naming server for clients\n");
        return 1;
    }
    printf("Naming server for clients listening on IP:%s\t Port:%d\n", ip_buffer, client_port);

    // Initialize naming server for storage servers
    int storage_server_sockfd = initialize_naming_server(ip_buffer, &storage_port);
    if (storage_server_sockfd == -1) {
        fprintf(stderr, "Failed to initialize naming server for storage servers\n");
        close(client_server_sockfd);
        return 1;
    }
    printf("Naming server for storage servers listening on IP:%s\t Port:%d\n", ip_buffer, storage_port);

    // Create a thread to accept client connections
    pthread_t client_accept_thread;
    if (pthread_create(&client_accept_thread, NULL, accept_client_connections, &client_server_sockfd) != 0) {
        perror("Failed to create accept thread for clients");
        close(client_server_sockfd);
        close(storage_server_sockfd);
        return 1;
    }

    // Create a thread to accept storage server connections
    pthread_t storage_accept_thread;
    if (pthread_create(&storage_accept_thread, NULL, accept_storage_connections, &storage_server_sockfd) != 0) {
        perror("Failed to create accept thread for storage servers");
        close(client_server_sockfd);
        close(storage_server_sockfd);
        return 1;
    }

    // Create a thread to periodically check server aliveness
    pthread_t alive_checker_thread;
    if (pthread_create(&alive_checker_thread, NULL, alive_checker, NULL) != 0) {
        perror("Failed to create alive checker thread");
        close(client_server_sockfd);
        close(storage_server_sockfd);
        return 1;
    }

    // Join the threads (optional, to keep main running indefinitely)
    pthread_join(client_accept_thread, NULL);
    pthread_join(storage_accept_thread, NULL);
    pthread_join(alive_checker_thread, NULL);

    close(client_server_sockfd);
    close(storage_server_sockfd);
    return 0;
}
