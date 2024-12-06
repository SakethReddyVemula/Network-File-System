#include "naming_server.h"

/**
 * @brief Periodically check if each storage server is still alive.
 * Disconnect the ones that have crashed.
 * Issue redundant delete and copy commands.
 *
 * @param arg NULL
 * @return void* NULL
 */
void *alive_checker(void *arg) {
    (void)arg;
    sleep(5);
    
    while (1) {
        sleep(15);  // Check every 5 seconds
        
        // Iterate through all storage servers
        for (int i = 0; i < MAX_SS; i++) {
            if (ss_infos[i]->port == -1) {
                continue;  // Skip inactive slots
            }
            
            // Create a new socket for checking connection
            int check_sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (check_sockfd == -1) {
                perror("Socket creation failed");
                continue;
            }
            
            // Set up connection address
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(ss_infos[i]->alive_checker_port); // online checker port
            addr.sin_addr.s_addr = inet_addr(ss_infos[i]->ip_address);  // has to be IP of storage server
            
            // Try to connect to the storage server
            if (connect(check_sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
                if (errno == ECONNREFUSED) {
                    printf("Storage server with port %d and index %d has disconnected!\n", ss_infos[i]->port, i);
                    
                    // Get all paths for this server before removing
                    char path_buffer[MAX_PATH_LENGTH];
                    memset(path_buffer, 0, sizeof(path_buffer));
                    print_trie(path_buffer, 0, root);
                    
                    // Delete all paths associated with this server from trie
                    // delete_server_paths(root, i);
                    
                    // Close the existing connection and mark server as inactive
                    close(ss_infos[i]->sockfd);
                    ss_infos[i]->sockfd = -1;
                    num_ss -= 1;
                    memset(ss_infos[i]->ip_address, 0, INET_ADDRSTRLEN);
                }
                else {
                    fprintf(stderr, "Connection failed with errno %d (%s)\n", 
                            errno, strerror(errno));
                    close(check_sockfd);
                    continue;
                }
            }
            
            close(check_sockfd);
        }
        handleBackup();
    }
    
    return NULL;
}