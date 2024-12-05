#include "naming_server.h"

int initialize_naming_server(char *ip_buffer, int *port) {
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        return -1;
    }

    // Zero out the server_addr structure and set address family
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on any local IP
    server_addr.sin_port = 0; // Let OS choose the port

    // Bind the socket to any IP and an available port
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket bind failed");
        close(sockfd);
        return -1;
    }

    // Get the actual IP and port assigned to the socket
    if (getsockname(sockfd, (struct sockaddr *)&server_addr, &addr_len) == -1) {
        perror("Getting socket name failed");
        close(sockfd);
        return -1;
    }

    // Find the device's IP address
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("Getting IP address failed");
        close(sockfd);
        return -1;
    }

    // Iterate over network interfaces to find a valid IP (ignores loopback)
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        if (ifa->ifa_addr->sa_family == AF_INET) { // Only look for IPv4 addresses
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            if (addr->sin_addr.s_addr != htonl(INADDR_LOOPBACK)) {
                strcpy(ip_buffer, inet_ntoa(addr->sin_addr));
                break;
            }
        }
    }
    freeifaddrs(ifaddr);

    // Check if an IP was found
    if (ip_buffer[0] == '\0') {
        fprintf(stderr, "No valid IP found\n");
        close(sockfd);
        return -1;
    }

    // Save the assigned port number
    *port = ntohs(server_addr.sin_port);

    // Start listening
    if (listen(sockfd, 5) == -1) {
        perror("Socket listen failed");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

void handleBackup()
{
    for(int i = 0; i < MAX_SS; i++)
    {
        if (ss_infos[i]->port == -1){
            continue;
        }
        if (ss_infos[i]->backup_ss_1 != -1 && ss_infos[ss_infos[i]->backup_ss_1]->sockfd == -1){
            ss_infos[i]->backup_ss_1 = -1;
        }
        if (ss_infos[i]->backup_ss_2 != -1 && ss_infos[ss_infos[i]->backup_ss_2]->sockfd == -1){
            ss_infos[i]->backup_ss_2 = -1;
        }
        if (ss_infos[i]->backup_ss_1 == -1 && ss_infos[i]->backup_ss_2 != -1){
            ss_infos[i]->backup_ss_1 = ss_infos[i]->backup_ss_2;
            ss_infos[i]->backup_ss_2 = -1;
        }
        if (num_ss >= 3){
            if (ss_infos[i]->backup_ss_2 == -1){
                for (int j = (i - 1 + MAX_SS) % MAX_SS; j != i; j = (j - 1 + MAX_SS) % MAX_SS){
                    if (ss_infos[j]->port != -1 && ss_infos[j]->sockfd != -1){
                        createBackup(i, j);
                        ss_infos[i]->backup_ss_2 = j;
                        break;
                    }
                }
            }
            if (ss_infos[i]->backup_ss_1 == -1){
                for (int j = (i - 1 + MAX_SS) % MAX_SS; j != i; j = (j - 1 + MAX_SS) % MAX_SS){
                    if (j == ss_infos[i]->backup_ss_2){
                        continue;
                    }
                    if (ss_infos[j]->port != -1 && ss_infos[j]->sockfd != -1){
                        createBackup(i, j);
                        ss_infos[i]->backup_ss_1 = j;
                        break;
                    }
                }
            }
            sleep(3);
        }
        
        

    }
}

// Function that handles each storage server connection
void *handle_storage_server(void *arg) {
    storage_info_t *storage_info = (storage_info_t *)arg;
    int storage_sockfd = storage_info->storage_sockfd;
    struct sockaddr_in storage_addr = storage_info->storage_addr;

    printf("Accepted connection from storage server %s:%d\n", 
           inet_ntoa(storage_addr.sin_addr), ntohs(storage_addr.sin_port));

    int buffer;
    LOG(storage_sockfd, -2, "Action: Waiting for ACK from SS");
    ssize_t bytes_received = recv(storage_sockfd, &buffer, sizeof(buffer), 0);
    LOG(storage_sockfd, bytes_received, "Action: Received ACK from SS");
    if (bytes_received <= 0) {
        perror("Failed to receive data from storage server");
        close(storage_sockfd);
        free(storage_info);
        return NULL;
    }

    if (buffer == 2) {
        printf("Received 2 from storage server\n");
    } else {
        printf("Unexpected data received from storage server\n");
    }

    // Declare a StorageServerInfo struct to receive data
    StorageServerInfo ss_info;
    LOG(storage_sockfd, -2, "Action: Waiting for StorageServerInfo from SS");
    bytes_received = recv(storage_sockfd, &ss_info, sizeof(ss_info), MSG_WAITALL);
    LOG(storage_sockfd, bytes_received, "Action: Received for StorageServerInfo from SS");
    if (bytes_received <= 0) {
        perror("Failed to receive data from storage server");
        close(storage_sockfd);
        free(storage_info);
        return NULL;
    }
    //  else if (bytes_received < sizeof(ss_info)) {
    //     printf("ss_info size: %ld\n bytes_received: %ld", sizeof(ss_info), bytes_received);
    //     printf("bytes received: %ld\n", bytes_received);
    //     fprintf(stderr, "Incomplete data received from storage server\n");
    //     close(storage_sockfd);
    //     free(storage_info);
    //     return NULL;
    // }
    int minIndex = -1;
    for (int i = 0; i < MAX_SS; i++){
        if (ss_infos[i]->sockfd == -1 && ss_infos[i]->port == ss_info.client_connection_port){
            minIndex = i;
            printf("Server number %d is back online\n", i);
            break;
        }
    }
    if (minIndex == -1){
        for (int i = 0; i < MAX_SS; i++){
            if (ss_infos[i]->port == -1){
                minIndex = i;
                break;
            }
        }
    }
    if (minIndex == -1){
        printf("Maximum number of storage servers reached\n");
        return NULL;
    }
    LOG(storage_sockfd, -2, "Action: Sending minIndex to SS");
    if (bytes_received = send(storage_sockfd, &minIndex, sizeof(minIndex), 0) == -1) {
        perror("Failed to send data to storage server");
    } else {
        printf("Sent 2 back to storage server\n");
    }
    LOG(storage_sockfd, bytes_received, "Action: Sent minIndex to SS");
    for (int i = 0; i < ss_info.path_count; i++){
        add_to_trie(ss_info.accessible_paths[i], minIndex);
    }

    char* new_buffer = (char*)malloc(sizeof(char) * MAX_STRING_SIZE);
    print_trie(new_buffer, 0, root);
    strcpy(ss_infos[minIndex]->ip_address, ss_info.ip);
    ss_infos[minIndex]->port = ss_info.client_connection_port;
    ss_infos[minIndex]->alive_checker_port = ss_info.alive_connection_port;
    ss_infos[minIndex]->sockfd = storage_sockfd;
    ss_infos[minIndex]->backed_up = 0;
    ss_infos[minIndex]->backup_ss_1 = -1;
    ss_infos[minIndex]->backup_ss_2 = -1;
    num_ss++;
    printf("The value of minindex is %d\n", minIndex);
    // Process received data
    printf("Received StorageServerInfo from storage server:\n");
    printf("IP: %s\n", ss_info.ip);
    printf("NM Connection Port: %d\n", ss_info.nm_connection_port);
    printf("Client Connection Port: %d\n", ss_info.client_connection_port);
    printf("Alive Checker Port: %d\n", ss_info.alive_connection_port);
    printf("Path Count: %d\n", ss_info.path_count);

    for (int i = 0; i < ss_info.path_count && i < MAX_PATHS; i++) {
        printf("Accessible Path %d: %s\n", i + 1, ss_info.accessible_paths[i]);
    }
    // close(storage_sockfd);
    free(storage_info);
    return NULL;
}


// Function that handles each client connection in a new thread
void *handle_client(void *arg) {
    client_info_t *client_info = (client_info_t *)arg;
    int client_sockfd = client_info->client_sockfd;
    struct sockaddr_in client_addr = client_info->client_addr;

    printf("Accepted connection from %s:%d\n", 
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Buffer to receive data from the client
    int buffer;
    
    // Receive a 1 from the client
    LOG(client_sockfd, -2, "Action: Receiving ACK from client");
    ssize_t bytes_received = recv(client_sockfd, &buffer, sizeof(buffer), 0);
    LOG(client_sockfd, bytes_received, "Action: Received ACK from client");
    if (bytes_received <= 0) {
        perror("Failed to receive data from client");
        close(client_sockfd);
        free(client_info);
        return NULL;
    }

    if (buffer == 1) {
        printf("Received 1 from client\n");

        // Send a 1 back to the client
        int response = 1;
        LOG(client_sockfd, -2, "Action: Sending ACK to client");
        if (bytes_received = send(client_sockfd, &response, sizeof(response), 0) == -1) {
            perror("Failed to send data to client");
        } else {
            printf("Sent 1 back to client\n");
        }
        LOG(client_sockfd, bytes_received, "Action: Sent ACK to client");
    } else {
        printf("Unexpected data received from client\n");
    }
    char* buff = (char*)(malloc(sizeof(char) * BUFFER_SIZE));
    while (1){
        

        client_to_NM request;
        LOG(client_sockfd, -2, "Action: Sending SS details to client");
        ssize_t bytesRec = recv(client_sockfd, &request, sizeof(request), 0);
        LOG(client_sockfd, bytesRec, "Action: Sent SS details to client");
        if (bytesRec <= 0){
            break;
        }
        if (request.oper == READ || request.oper == GETINFO || request.oper == WRITE || request.oper == STREAM){
            NM_to_client res;
            // printf("%s\n", request.file_path);
            int index = get(cache, request.file_path);
            if (index != -1){
                printf("Cache hit!\n");
            }
            else{
                index = get_storage_server_index(request.file_path);
            }            
            if (index == -1){
                res.e_code = INVALID_PATH;
                LOG(client_sockfd, -2, "Action: Sending ERROR details to client");
                bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                LOG(client_sockfd, bytesRec, "Action: Sent ERROR details to client");
                continue;
            }
            if (ss_infos[index]->sockfd == -1){
                res.e_code = MAIN_SERVER_DOWN;
                // send(client_sockfd, &res, sizeof(res), 0);
                if (request.oper == READ){
                    if (ss_infos[index]->backup_ss_1 == -1){
                        res.e_code = ALL_BACKUPS_DOWN;
                        LOG(client_sockfd, -2, "Action: Sending ERROR details to client");
                        bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                        LOG(client_sockfd, bytesRec, "Action: Sent ERROR details to client");
                        continue;
                    }
                    LOG(client_sockfd, -2, "Action: Sending ERROR details to client");
                    bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                    LOG(client_sockfd, bytesRec, "Action: Sent ERROR details to client");
                    char backuppath[MAX_PATH_LENGTH];
                    snprintf(backuppath, sizeof(backuppath), "backup_%d/%s", ss_infos[index]->backup_ss_1, request.file_path);
                    strcpy(request.file_path, backuppath);
                    LOG(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, -2, "Action: Sending clinet_to_NM struct to backup");
                    bytesRec = send(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &request, sizeof(request), 0);
                    LOG(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, bytesRec, "Action: Sending clinet_to_NM struct to backup");
                    SS_to_NM_struct data;

                    // while (1) {
                    //     LOG(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, -2, "Action: Receiving SS_to_NM_struct struct to backup");
                    //     if (recv(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &data, sizeof(data), 0) <= 0) {
                    //         return NULL;
                    //     }
                    //     LOG(client_sockfd, bytesRec, "Action: Received SS_to_NM_struct struct to backup");
                    //     LOG(client_sockfd, -2, "Action: Sending SS_to_NM_struct struct to client");
                    //     if (bytesRec = send(client_sockfd, &data, sizeof(data), 0) < 0) {
                    //         return NULL;
                    //     }
                    //     LOG(client_sockfd, bytesRec, "Action: Sent SS_to_NM_struct struct to client");
                    //     if (data.stop) {
                    //         break;
                    //     }
                    // }
                    memset(data.buffer, 0, sizeof(data.buffer));
                    while (strlen(data.buffer) == 0){
                        recv(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &data, sizeof(data), MSG_WAITALL);
                    }
                    printf("Backup data %s received\n", data.buffer);
                    send(client_sockfd, &data, sizeof(data), 0);
                    continue;
                }
                else{
                    LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                    bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                    LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
                }
                continue;
            }
            else{
                if (ss_infos[index]->sockfd == -1 && request.oper == READ){
                    if (ss_infos[index]->backup_ss_1 != -1){
                        res.e_code = MAIN_SERVER_DOWN;
                    }
                    else{
                        res.e_code = ALL_BACKUPS_DOWN;
                    }
                    LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                    bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                    LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
                    // Read from backup and send to client
                    continue;
                }
                else if (ss_infos[index]->sockfd == -1){
                    res.e_code = MAIN_SERVER_DOWN;
                    LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                    bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                    LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
                    continue;
                }
                else{
                    if (request.oper == WRITE){
                        client_to_NM replicate;
                        if (ss_infos[index]->backup_ss_1 != -1){
                            char filepath[MAX_PATH_LENGTH];
                            snprintf(filepath, sizeof(filepath), "backup_%d/%s", ss_infos[index]->backup_ss_1, request.file_path);
                            replicate.oper = WRITE;
                            strcpy(replicate.file_path, filepath);
                            printf("Sent replication request to %d server\n", ss_infos[index]->backup_ss_1);
                            send(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &replicate, sizeof(replicate), 0);
                            int ack;
                            recv(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &ack, sizeof(ack), MSG_WAITALL);
                        }
                        if (ss_infos[index]->backup_ss_2 != -1){
                            char filepath[MAX_PATH_LENGTH];
                            snprintf(filepath, sizeof(filepath), "backup_%d/%s", ss_infos[index]->backup_ss_2, request.file_path);
                            replicate.oper = WRITE;
                            strcpy(replicate.file_path, filepath);
                            printf("Sent replication request to %d server\n", ss_infos[index]->backup_ss_2);
                            send(ss_infos[ss_infos[index]->backup_ss_2]->sockfd, &replicate, sizeof(replicate), 0);
                            int ack;
                            recv(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &ack, sizeof(ack), MSG_WAITALL);
                        }

                    }
                    put(cache, request.file_path, index);
                    strcpy(res.IP_SS, ss_infos[index]->ip_address);
                    res.port_SS = ss_infos[index]->port;
                    res.e_code = SUCCESS;
                }
                

            }
            LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
            bytesRec = send(client_sockfd, &res, sizeof(res), 0);
            LOG(client_sockfd, bytesRec, "Action: Sending NM_to_client struct to client");
        }
        else if (request.oper == DELETE){
            NM_to_client res;
            
            int index = delete_file_path(request.file_path);
            
           
            int ack = 1;
            if (index == -1){
                res.e_code = INVALID_PATH;
                LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
                continue;
            }
            if (ss_infos[index]->sockfd == -1){
                res.e_code = MAIN_SERVER_DOWN;
                LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
                continue;
            }
            else{
                if (ss_infos[index]->sockfd == -1){
                    res.e_code = MAIN_SERVER_DOWN;
                    LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                    bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                    LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
                    continue;
                }
                removeFromCache(cache, request.file_path);
                printf("Sent delete to ss\n");
                LOG(ss_infos[index]->sockfd, -2, "Action: Sending client_to_NM struct to SS");
                bytesRec = send(ss_infos[index]->sockfd, &request, sizeof(request), 0);
                LOG(ss_infos[index]->sockfd, bytesRec, "Action: Sent client_to_NM struct to SS");
                LOG(ss_infos[index]->sockfd, -2, "Action: Receiving ack to SS");
                bytesRec = recv(ss_infos[index]->sockfd, &ack, sizeof(int), 0);
                strcpy(res.IP_SS, ss_infos[index]->ip_address);
                LOG(ss_infos[index]->sockfd, bytesRec, "Action: Recieved NM_to_client struct to SS");
                res.port_SS = ss_infos[index]->port;
                res.e_code = SUCCESS;

                client_to_NM replicate;
                if (ss_infos[index]->backup_ss_1 != -1){
                    char filepath[MAX_PATH_LENGTH];
                    snprintf(filepath, sizeof(filepath), "backup_%d/%s", ss_infos[index]->backup_ss_1, request.file_path);
                    replicate.oper = DELETE;
                    strcpy(replicate.file_path, filepath);
                    printf("Sent replication request to %d server\n", ss_infos[index]->backup_ss_1);
                    send(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &replicate, sizeof(replicate), 0);
                    int ack;
                    recv(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &ack, sizeof(ack), MSG_WAITALL);
                }
                if (ss_infos[index]->backup_ss_2 != -1){
                    char filepath[MAX_PATH_LENGTH];
                    snprintf(filepath, sizeof(filepath), "backup_%d/%s", ss_infos[index]->backup_ss_2, request.file_path);
                    replicate.oper = DELETE;
                    strcpy(replicate.file_path, filepath);
                    printf("Sent replication request to %d server\n", ss_infos[index]->backup_ss_2);
                    send(ss_infos[ss_infos[index]->backup_ss_2]->sockfd, &replicate, sizeof(replicate), 0);
                    int ack;
                    recv(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &ack, sizeof(ack), MSG_WAITALL);
                }

            }
            LOG(client_sockfd, -2, "Action: Receiving ACK to client");
            bytesRec = send(client_sockfd, &ack, sizeof(ack), 0);
            LOG(client_sockfd, bytesRec, "Action: Received ACK to client");
        }
        else if (request.oper == CREATE){
            NM_to_client res;
            
            
            int index = createNewFile(request.file_path, request.file_path_2);

            if (index == -1){
                res.e_code = INVALID_PATH;
                LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
                continue;
            }
            if (ss_infos[index]->sockfd == -1){
                res.e_code = MAIN_SERVER_DOWN;
                LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
                continue;
            }
            else{
                if (ss_infos[index]->sockfd == -1){
                    res.e_code = MAIN_SERVER_DOWN;
                    LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
                    bytesRec = send(client_sockfd, &res, sizeof(res), 0);
                    LOG(client_sockfd, bytesRec, "Action: Sending NM_to_client struct to client");
                    continue;
                }
                client_to_NM replicate;
                if (ss_infos[index]->backup_ss_1 != -1){
                    char filepath[MAX_PATH_LENGTH];
                    snprintf(filepath, sizeof(filepath), "backup_%d/%s", ss_infos[index]->backup_ss_1, request.file_path);
                    replicate.oper = CREATE;
                    strcpy(replicate.file_path, filepath);
                    printf("Sent replication request to %d server\n", ss_infos[index]->backup_ss_1);
                    send(ss_infos[ss_infos[index]->backup_ss_1]->sockfd, &replicate, sizeof(replicate), 0);
                }
                if (ss_infos[index]->backup_ss_2 != -1){
                    char filepath[MAX_PATH_LENGTH];
                    snprintf(filepath, sizeof(filepath), "backup_%d/%s", ss_infos[index]->backup_ss_2, request.file_path);
                    replicate.oper = CREATE;
                    strcpy(replicate.file_path, filepath);
                    printf("Sent replication request to %d server\n", ss_infos[index]->backup_ss_2);
                    send(ss_infos[ss_infos[index]->backup_ss_2]->sockfd, &replicate, sizeof(replicate), 0);
                }
                put(cache, request.file_path, index);
                LOG(ss_infos[index]->sockfd, -2, "Action: Sending client_to_NM struct to SS");
                bytesRec = send(ss_infos[index]->sockfd, &request, sizeof(request), 0);
                LOG(ss_infos[index]->sockfd, bytesRec, "Action: Sending client_to_NM struct to SS");
                int ack;
                // recv(ss_infos[index]->sockfd, &ack, sizeof(int), 0);
                res.e_code = SUCCESS;
                res.ack = 1;
            }
            LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to client");
            bytesRec = send(client_sockfd, &res, sizeof(res), 0);
            LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
        }
        else if (request.oper == COPY){
            NM_to_client res;
            int err = handle_copy(request.file_path_2, request.file_path);
            res.e_code = err;
            res.ack = 1;
            LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
            send(client_sockfd, &res, sizeof(res), 0);
            LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to client");
        }
        else if (request.oper == LIST){
            // printf("List enc\n");
            int err;
            if (strcmp(request.file_path, "/") == 0){
                send_trie_optimized(root, client_sockfd);
                err = SUCCESS;
            }
            else{
                err = search_trie(buff, 0, request.file_path, root, client_sockfd);
            }
            NM_to_client res;
            res.e_code = err;
            res.ack = 1;
            LOG(client_sockfd, -2, "Action: Sending NM_to_client struct to SS");
            bytesRec = send(client_sockfd, &res, sizeof(res), 0);
            LOG(client_sockfd, bytesRec, "Action: Sent NM_to_client struct to SS");
            printf("Sent ack to client\n");
            // printf("Size of nm_to_client %lu\n", sizeof(res));
        }
        print_trie(buff, 0, root);


    }

    // Close the client connection and free memory
    close(client_sockfd);
    free(client_info);
    return NULL;
}


// Thread that listens and accepts new connections
void *accept_client_connections(void *arg) {
    int server_sockfd = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        // Accept new connections
        int client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sockfd == -1) {
            perror("Accept failed");
            continue;
        }

        // Allocate memory for client info
        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (client_info == NULL) {
            perror("Malloc failed");
            close(client_sockfd);
            continue;
        }
        client_info->client_sockfd = client_sockfd;
        client_info->client_addr = client_addr;

        // Create a thread to handle the client
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, client_info) != 0) {
            perror("Failed to create thread for client");
            free(client_info);
            close(client_sockfd);
            continue;
        }

        pthread_detach(client_thread); // Detach the thread to clean up resources automatically
    }

    return NULL;
}




// Thread that listens and accepts new connections for storage servers
void *accept_storage_connections(void *arg) {
    int storage_sockfd = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        int client_sockfd = accept(storage_sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sockfd == -1) {
            perror("Accept failed");
            continue;
        }

        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (client_info == NULL) {
            perror("Malloc failed");
            close(client_sockfd);
            continue;
        }
        client_info->client_sockfd = client_sockfd;
        client_info->client_addr = client_addr;

        pthread_t storage_thread;
        if (pthread_create(&storage_thread, NULL, handle_storage_server, client_info) != 0) {
            perror("Failed to create thread for storage server");
            free(client_info);
            close(client_sockfd);
            continue;
        }

        pthread_detach(storage_thread);
    }

    return NULL;
}