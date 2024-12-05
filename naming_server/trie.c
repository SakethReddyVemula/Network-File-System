#include "naming_server.h"


// A mapping of characters to the index they are stored in the trie. a-z will be 0-25 and / will be 26
int char_to_index_mapping(char c)
{
    // if (c == '/'){
    //     return 26;
    // }
    // else if (c == '.'){
    //     return 27;
    // }
    // else{
    //     return c - 'a';
    // }
    return c;
}

// Create a new trie node.
trieNode *createTrieNode(char c)
{
    trieNode *new = (trieNode *)(malloc(sizeof(trieNode)));
    new->value = c;
    new->children = (trieNode **)(malloc(sizeof(trieNode *) * CHARS));
    for (int i = 0; i < CHARS; i++)
    {
        new->children[i] = NULL;
    }
    new->ss_index = -1;
    return new;
}

// Add the file path to the trie, given the file path and the index of the storage server. Returns 0 on success and -1 on failure
int add_to_trie(char *file_path, int index)
{
    trieNode *curr = root;
    for (int i = 0; file_path[i] != '\0'; i++)
    {
        if (!curr->children[char_to_index_mapping(file_path[i])])
        {
            curr->children[char_to_index_mapping(file_path[i])] = createTrieNode(file_path[i]);
        }
        curr = curr->children[char_to_index_mapping(file_path[i])];
        if (curr->value == '/')
        {
            if (curr->ss_index != -1 && curr->ss_index != index)
            {
                return -1;
            }
            curr->ss_index = index;
        }
    }
    if (curr->ss_index != -1)
    {
        return -1;
    }
    else
    {
        curr->ss_index = index;
        return 0;
    }
}

// Returns the storage server index. -1 on failure
int get_storage_server_index(char *file_path)
{
    trieNode *curr = root;
    for (int i = 0; file_path[i] != '\0'; i++)
    {
        if (!curr->children[char_to_index_mapping(file_path[i])])
        {
            return -1;
        }
        curr = curr->children[char_to_index_mapping(file_path[i])];
    }

    return curr->ss_index;
}

// Deletes file from the trie. Returns 0 on success and -1 on failure
int delete_file_path(char *file_path)
{
    trieNode *curr = root;
    int i = 0;
    for (i; file_path[i] != '\0'; i++)
    {
        if (!curr->children[char_to_index_mapping(file_path[i])])
        {
            return -1;
        }
        curr = curr->children[char_to_index_mapping(file_path[i])];
    }
    if (curr->ss_index == -1)
    {
        return -1;
    }
    if (file_path[i - 1] == '/')
    {
        for (int j = 0; j < CHARS; j++)
        {
            curr->children[j] = NULL;
        }
    }
    
    int temp = curr->ss_index;
    curr->ss_index = -1;
    return temp;

}

void print_trie(char *buffer, int size, trieNode *curr)
{
    if (curr == NULL)
    {
        return;
    }
    buffer[size] = curr->value;
    buffer[size + 1] = '\0';
    if (curr->ss_index != -1)
    {
        printf("%s, %d\n", buffer, curr->ss_index);
    }
    for (int i = 0; i < CHARS; i++)
    {
        if (curr->children[i] != NULL)
        {
            print_trie(buffer, size + 1, curr->children[i]);
        }
    }
}

void collect_trie_data(char *buffer, int size, trieNode *curr, char *output, int *output_size) {
    if (curr == NULL) {
        return;
    }
    buffer[size] = curr->value;
    buffer[size + 1] = '\0';

    if (curr->ss_index != -1) {
        strcat(output, buffer);
        strcat(output, "\n");
        *output_size += strlen(buffer) + 1;
    }
    for (int i = 0; i < CHARS; i++) {
        if (curr->children[i] != NULL) {
            collect_trie_data(buffer, size + 1, curr->children[i], output, output_size);
        }
    }
}

void send_trie_optimized(trieNode *curr, int sockfd) {
    char buffer[1024];
    char output[BUFFER_SIZE * 10];
    memset(output, 0, sizeof(output));
    int output_size = 0;

    collect_trie_data(buffer, 0, curr, output, &output_size);
    unsigned int output_len = strlen(output);
    output[output_len] = '\n';
    output[output_len + 1] = '\n';
    output[output_len + 2] = '\n';
    output[output_len + 3] = '\0';
    printf("Sending LIST:\n%s\n", output);
    // send(sockfd, output, sizeof(output), 0);
    LOG(sockfd, -2, "Action: Sending Trie to client");
    ssize_t log_result = send(sockfd, output, sizeof(output), 0);
    printf("Log Sent: %lu\n", log_result);
    LOG(sockfd, log_result, "Action: Sent Trie to client");

    // char stop_signal[] = "STOP";
    // send(sockfd, stop_signal, strlen(stop_signal), 0);

}




int search_trie(char* buffer, int size, char* file_path, trieNode *curr, int sockfd)
{   
    int error_code;
    if (curr == NULL)
    {
        return INVALID_PATH;
    }
    buffer[size] = curr->value;
    buffer[size + 1] = '\0';
    if (curr->ss_index != -1)
    {
        if (strcmp(buffer, file_path) == 0)
        {   
            // printf("Found him\n");
            send_trie_optimized(curr, sockfd);
            // char stop_signal[] = "STOP";
            // send(sockfd, stop_signal, strlen(stop_signal), 0);
            return SUCCESS;
        }
        // printf("%s\n", buffer);
    }
    for (int i = 0; i < CHARS; i++)
    {
        if (curr->children[i] != NULL)
        {
            print_trie(buffer, size + 1, curr->children[i]);
            error_code = search_trie(buffer, size + 1, file_path, curr->children[i], sockfd);
            if (error_code == SUCCESS){
                return SUCCESS;
            }
        }
    }

    return INVALID_PATH;

}



int createNewFile(char *filePath, char *filePath2)
{
    if (filePath[strlen(filePath) - 1] != '/')
    {
        printf("Error 1\n");
        return -1;
    }
    trieNode *curr = root;
    for (int i = 0; filePath[i] != '\0'; i++)
    {
        if (curr->children[filePath[i]] == NULL)
        {
            printf("Error 2\n");
            return -1;
        }
        curr = curr->children[filePath[i]];
    }
    int idx = curr->ss_index;
    for (int i = 0; filePath2[i] != '\0'; i++)
    {
        if (curr->children[filePath2[i]] == NULL)
        {
            curr->children[filePath2[i]] = createTrieNode(filePath2[i]);
        }
        curr = curr->children[filePath2[i]];
    }
    if (curr->ss_index != -1)
    {
        printf("Error 3");
        return -1;
    }
    curr->ss_index = idx;
    return curr->ss_index;
}

// int handle_copy(char *dest, char *src) {
//     // Check if destination ends with '/'
//     if (dest[strlen(dest) - 1] != '/') {
//         return INVALID_PATH;
//     }
    
//     // Find last slash in source path
//     int lastSlash = -1;
//     for (int i = 0; src[i] != '\0'; i++) {
//         if (src[i] == '/') {
//             lastSlash = i;
//         }
//     }
//     if (lastSlash == -1) {
//         return INVALID_PATH;
//     }
    
//     // Validate storage server indices
//     int srcIdx = get_storage_server_index(src);
//     if (srcIdx == -1) {
//         return INVALID_PATH;
//     }
    
//     int destIdx = get_storage_server_index(dest);
//     if (destIdx == -1) {
//         return INVALID_PATH;
//     }
    
//     client_to_NM ss_operation;
//     int ack;
    
//     // Create operation for destination
//     ss_operation.oper = CREATE;
//     strcpy(ss_operation.file_path, dest);
//     strcpy(ss_operation.file_path_2, src + lastSlash + 1);
//     if (send(ss_infos[destIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0) < 0) {
//         return INVALID_PATH;
//     }
//     if (recv(ss_infos[destIdx]->sockfd, &ack, sizeof(int), 0) <= 0) {
//         return INVALID_PATH;
//     }
    
//     // Read operation from source
//     ss_operation.oper = READ;
//     strcpy(ss_operation.file_path, src);
//     if (send(ss_infos[srcIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0) < 0) {
//         return INVALID_PATH;
//     }
    
//     // Write operation to destination
//     ss_operation.oper = WRITE;
//     strcpy(ss_operation.file_path, dest);
//     strcat(ss_operation.file_path, src + lastSlash + 1);
    
//     // Update the trie with new path
//     add_to_trie(ss_operation.file_path, destIdx);
    
//     if (send(ss_infos[destIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0) < 0) {
//         return INVALID_PATH;
//     }
    
//     // Forward all data
//     SS_to_NM_struct data;
//     while (1) {
//         ssize_t bytes_received = recv(ss_infos[srcIdx]->sockfd, &data, sizeof(data), 0);
//         if (bytes_received <= 0) {
//             return INVALID_PATH;
//         }
        
//         if (data.stop) {
//             // Forward the stop signal
//             if (send(ss_infos[destIdx]->sockfd, &data, sizeof(data), 0) < 0) {
//                 return INVALID_PATH;
//             }
//             break;
//         }
        
//         // Forward the data
//         if (send(ss_infos[destIdx]->sockfd, &data, sizeof(data), 0) < 0) {
//             return INVALID_PATH;
//         }
//     }
    
//     // Wait for final acknowledgment
//     if (recv(ss_infos[destIdx]->sockfd, &ack, sizeof(int), 0) <= 0) {
//         return INVALID_PATH;
//     }
    
//     return SUCCESS;
// }

int handle_copy(char *dest, char *src) {
    if (dest[strlen(dest) - 1] != '/') {
        return INVALID_PATH;
    }
    
    int srcIdx = get_storage_server_index(src);
    if (srcIdx == -1) {
        return INVALID_PATH;
    }
    
    int destIdx = get_storage_server_index(dest);
    if (destIdx == -1) {
        return INVALID_PATH;
    }
    if (ss_infos[srcIdx]->sockfd == -1 || ss_infos[destIdx]->sockfd == -1){
        return MAIN_SERVER_DOWN;
    }
    
    client_to_NM ss_operation;
    if (src[strlen(src) - 1] == '/'){
        printf("Handling dir now\n");
        handle_dir(dest, src,destIdx, srcIdx);
        return SUCCESS;
    }
    ss_operation.oper = READ;
    strcpy(ss_operation.file_path, src);
    // send(ss_infos[srcIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);

    LOG(ss_infos[srcIdx]->sockfd, -2, "Action: Sending READ req to Source SS");
    ssize_t log_result = send(ss_infos[srcIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);
    LOG(ss_infos[srcIdx]->sockfd, log_result, "Action: Sent READ req to Source SS");

    
    SS_to_NM_struct data;
    memset(data.buffer, 0, sizeof(data.buffer));
    while(strlen(data.buffer) == 0){
        LOG(ss_infos[srcIdx]->sockfd, -2, "Action: Waiting for copy data from source SS");
        ssize_t log_result = recv(ss_infos[srcIdx]->sockfd, &data, sizeof(data), 0);
        LOG(ss_infos[srcIdx]->sockfd, log_result, "Action: Copied data from source SS");
        
        printf("Data is %s and \n", data.buffer);
    }
    // Write operation
    ss_operation.oper = WRITE;
    strcpy(ss_operation.file_path, dest);
    
    
    // Extract the source name for the destination path
    char *last_slash = strrchr(src, '/');
    if (last_slash) {
        strcat(ss_operation.file_path, last_slash + 1);
    }
    add_to_trie(ss_operation.file_path, destIdx);
    LOG(ss_infos[destIdx]->sockfd, -2, "Action: Sending Trie to client");
    log_result = send(ss_infos[destIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);
    LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Sent Trie to client");
    if (log_result < 0) {
        return INVALID_PATH;
    }

    
    
    // memset(data.buffer, 0, sizeof(data.buffer));
    
    // Read operation

    // while (strlen(data.buffer) == 0)
    // {
        
        
    // }
    printf("The received data is %s\n", data.buffer);
    LOG(ss_infos[destIdx]->sockfd, -2, "Action: Writing copy data to dest SS");
    log_result = send(ss_infos[destIdx]->sockfd, &data, sizeof(data), 0);
    LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Written Data copy data to dest SS");

    // send(ss_infos[destIdx]->sockfd, &data, sizeof(data), 0);

    
    // Forward all data
   
    // while (1) {
    //     if (recv(ss_infos[srcIdx]->sockfd, &data, sizeof(data), 0) <= 0) {
    //         printf("Haha recv returned bad");
    //         return INVALID_PATH;
    //     }
    //     printf("Data received: %s\n and stop value of %d\n", data.buffer, data.stop);
    //     if (send(ss_infos[destIdx]->sockfd, &data, sizeof(data), 0) < 0) {
    //         return INVALID_PATH;
    //     }
        
    //     if (data.stop == 1) {
    //         printf("Breaking from data transfer\n");
    //         break;
    //     }
    // }
   
     
    int ack;
    // recv(ss_infos[destIdx]->sockfd, &ack, sizeof(int), 0);
    LOG(ss_infos[destIdx]->sockfd, -2, "Action: Waiting for ack from destination SS");
    log_result = recv(ss_infos[destIdx]->sockfd, &ack, sizeof(int), 0);
    LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Acknowledgement received from destination SS");
    printf("Acknowledgement %d for copy received\n", ack);
    return SUCCESS;
}


void handle_dir(char* dest, char* src, int destIdx, int srcIdx){
    int lastSlash = -1;
    for (int i = 0; i < strlen(src) - 1; i++){
        if (src[i] == '/'){
            lastSlash = i;
        }
    }

    client_to_NM ss_operation;
    ss_operation.oper = CREATE;
    strcpy(ss_operation.file_path, dest);
    strcpy(ss_operation.file_path_2, src + lastSlash + 1);
    // send(ss_infos[destIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);

    LOG(ss_infos[destIdx]->sockfd, -2, "Action: Sending CREATE req to Dest SS");
    ssize_t log_result = send(ss_infos[destIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);
    LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Sent CREATE req to Dest SS");
    

    trieNode* curr = root;
    for (int i = 0; i < strlen(src); i++){
        curr = curr->children[src[i]];
    }
    char* buffer = (char*)(malloc(sizeof(char) * MAX_PATH_LENGTH));
    // curr->ss_index = destIdx;
    // handle_dirs_rec(buffer, 0, curr);
    char newdest[MAX_PATH_LENGTH];
    strcpy(newdest, dest);
    strcat(newdest, src + lastSlash + 1);
    add_to_trie(newdest, destIdx);
    printf("The value of new dest is %s\n", newdest);
    for (int i = 0; i < CHARS; i++){
        if (curr->children[i]){
            handle_dirs_rec(buffer, 0, curr->children[i], srcIdx, destIdx, src, newdest);
        }
    }

}

void handle_dirs_rec(char* buffer, int depth, trieNode* curr, int srcIdx, int destIdx, char* src, char* dest){
    buffer[depth] = curr->value;
    buffer[depth + 1] = '\0';

    if (curr->ss_index != -1) {
        if (curr->value == '/') {
            // Handle subdirectory
            char new_dest[MAX_PATH_LENGTH];
            strcpy(new_dest, dest);

            char new_src[MAX_PATH_LENGTH];
            strcpy(new_src, src);
            strcat(new_src, buffer);

            handle_dir(new_dest, new_src, destIdx, srcIdx);
            return;
        } else {
            // Handle file copy
            char file_src[MAX_PATH_LENGTH];
            strcpy(file_src, src);
            strcat(file_src, buffer);

            char file_dest[MAX_PATH_LENGTH];
            strcpy(file_dest, dest);
            printf("New dest: %s, new src: %s\n", file_dest, file_src);
            handle_copy(file_dest, file_src);
        }
    }

    // Recurse for children
    for (int i = 0; i < CHARS; i++) {
        if (curr->children[i]) {
            handle_dirs_rec(buffer, depth + 1, curr->children[i], srcIdx, destIdx, src, dest);
        }
    }
}

void createBackup(int srcIdx, int destIdx){
    trieNode* curr = root;
    char* buffer = (char*)(malloc(sizeof(char) * MAX_PATH_LENGTH));
    for (int i = 0; i < CHARS; i++){
        if (curr->children[i] != NULL){
            copy_to_backup_server(curr->children[i], buffer, 0, srcIdx, destIdx);
        }
    }
}

void copy_to_backup_server(trieNode* curr, char* buffer, int in, int srcIdx, int destIdx){
    buffer[in] = curr->value;
    buffer[in + 1] = '\0';
    if (curr->ss_index == srcIdx){
        if (curr->value != '/'){
            client_to_NM ss_operation;
            
            // Read operation
            ss_operation.oper = READ;
            strcpy(ss_operation.file_path, buffer);
            printf("Sending path %s for read\n", ss_operation.file_path);

            LOG(ss_infos[destIdx]->sockfd, -2, "Action: Sending READ req to source SS");
            ssize_t log_result = send(ss_infos[srcIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);
            LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Sent READ req to source SS");
            
            if (log_result < 0) {
                return;
            }
            SS_to_NM_struct data;
            memset(data.buffer, 0, sizeof(data.buffer));
            // while (strlen(data.buffer) == 0){
                // recv(ss_infos[srcIdx]->sockfd, &data, sizeof(data), 0);
                LOG(ss_infos[srcIdx]->sockfd, -2, "Action: Copying data from source SS");
                log_result = recv(ss_infos[srcIdx]->sockfd, &data, sizeof(data), MSG_WAITALL);
                LOG(ss_infos[srcIdx]->sockfd, log_result, "Action: Received data from source SS");
            // }
            
            
            printf("Received data %s\n", data.buffer);
            
            // Write operation
            ss_operation.oper = WRITE;
            snprintf(ss_operation.file_path, sizeof(ss_operation.file_path), "backup_%d/", destIdx);
            strcat(ss_operation.file_path, buffer);
            printf("Sending path %s for write\n", ss_operation.file_path);
            
            LOG(ss_infos[destIdx]->sockfd, -2, "Action: Sending WRITE req to dest SS");
            log_result = send(ss_infos[destIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);
            LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Sent WRITE req to dest SS");
            if (log_result < 0) {
                return;
            }

            LOG(ss_infos[destIdx]->sockfd, -2, "Action: Writing data to Backup");
            log_result = send(ss_infos[destIdx]->sockfd, &data, sizeof(data), 0);
            LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Wrote data to Backup");

            // send(ss_infos[destIdx]->sockfd, &data, sizeof(data), 0);
            
            // Forward all data
            // SS_to_NM_struct data;
            // while (1) {
            //     if (recv(ss_infos[srcIdx]->sockfd, &data, sizeof(data), 0) <= 0) {
            //         return;
            //     }
                
            //     if (send(ss_infos[destIdx]->sockfd, &data, sizeof(data), 0) < 0) {
            //         return;
            //     }
                
            //     if (data.stop) {
            //         break;
            //     }
            // }
            
            int ack;
            // recv(ss_infos[destIdx]->sockfd, &ack, sizeof(int), 0);

            LOG(ss_infos[destIdx]->sockfd, -2, "Action: Receiving ack from Dest SS");
            log_result = recv(ss_infos[destIdx]->sockfd, &ack, sizeof(int), 0);
            LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Received ack from Dest SS");
        }
        else{
            client_to_NM ss_operation;
            ss_operation.oper = CREATE;
            // strcpy(ss_operation.file_path, dest);
            // strcpy(ss_operation.file_path_2, src + lastSlash + 1);
            int lastSlash = -1;
            for (int i = 0; i < strlen(buffer) - 1; i++){
                if (buffer[i] == '/'){
                    lastSlash = i;
                }
            }
            snprintf(ss_operation.file_path, sizeof(ss_operation.file_path), "backup_%d/", destIdx);
            strncat(ss_operation.file_path, buffer, lastSlash + 1);
            strcpy(ss_operation.file_path_2, buffer + lastSlash + 1);
            printf("Sending %s and %s for create\n", ss_operation.file_path, ss_operation.file_path_2);
            // send(ss_infos[destIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);

            LOG(ss_infos[destIdx]->sockfd, -2, "Action: Sending Copy-CREATE req to Dest SS");
            ssize_t log_result = send(ss_infos[destIdx]->sockfd, &ss_operation, sizeof(ss_operation), 0);
            LOG(ss_infos[destIdx]->sockfd, log_result, "Action: Sent Copy-CREATE req to Dest SS");

            
        }
    }
    for (int i = 0; i < CHARS; i++){
        if (curr->children[i] != NULL){
            copy_to_backup_server(curr->children[i], buffer, in + 1, srcIdx, destIdx);
        }
    }
}
