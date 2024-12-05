#include "storage_server.h"

int nm_sockfd, client_sockfd, alive_sockfd;
file_lock_manager_t lock_manager; 
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void send_error_to_client(int client_socket, const char *error_msg)
{
    // send(client_socket, error_msg, strlen(error_msg), 0);
}

void create_file_or_directory(const char *path)
{
    // Check if the path ends with '/'
    size_t len = strlen(path);
    if (len > 0 && path[len - 1] == '/')
    {
        // Create a directory
        if (mkdir(path, 0755) == -1)
        {
            printf("ERROR creating directory at %s\n", path);
        }
        else
        {
            printf("Directory created at %s\n", path);
        }
    }
    else
    {
        // Create a file
        int fd = open(path, O_CREAT | O_WRONLY, 0644); // Create file with write permissions
        if (fd == -1)
        {
            perror("ERROR creating file");
        }
        else
        {
            printf("File created at %s\n", path);
            close(fd); // Close the file descriptor
        }
    }
}

void delete_directory(const char *path)
{
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL)
    {
        perror("ERROR opening directory");
        return;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

        struct stat statbuf;
        if (stat(filepath, &statbuf) == -1)
        {
            perror("ERROR retrieving file stats");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode))
        {
            delete_directory(filepath);
        }
        else
        {
            if (remove(filepath) == -1)
            {
                perror("ERROR deleting file");
            }
        }
    }
    closedir(dp);

    if (rmdir(path) == -1)
    {
        perror("ERROR removing directory");
    }
}

void delete_file_or_directory(const char *path)
{
    struct stat statbuf;
    if (stat(path, &statbuf) == -1)
    {
        perror("ERROR retrieving file stats");
        return;
    }

    if (S_ISDIR(statbuf.st_mode))
    {
        delete_directory(path);
    }
    else
    {
        if (remove(path) == -1)
        {
            perror("ERROR deleting file");
        }
    }
}

void read_file(int client_socket, const char *path)
{
    pthread_rwlock_t* file_lock = get_file_lock(path);
    if(!file_lock){
        send_error_to_client(client_socket, "Failed to aquire file lock");
        return;
    }

    // Acquire read lock
    if(pthread_rwlock_rdlock(file_lock) != 0){
        send_error_to_client(client_socket, "Failed to acquire read lock");
        return;
    }

    printf("%s\n", path);
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        // pthread_rwlock_unlock(file_lock);
        perror("ERROR opening file for reading");
        send_error_to_client(client_socket, "Failed to open file");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0)
    {
        buffer[bytes] = '\0';
        printf("%s\n", buffer);
        // pthread_mutex_lock(&lock);
        if (send(client_socket, buffer, sizeof(buffer), 0) < 0)
        {
            perror("ERROR sending file data to client");
            break;
        }
        // pthread_mutex_unlock(&lock);
        memset(buffer, 0, BUFFER_SIZE);
    }

    close(fd);
    pthread_rwlock_unlock(file_lock);
}

void nm_read_file(int nm_socket, const char *path)
{
    // pthread_rwlock_t* file_lock = get_file_lock(path);
    // if(!file_lock){
    //     printf("Inside\n");
    //     send_error_to_client(nm_socket, "Failed to aquire file lock");
    //     return;
    // }
    // printf("Outside\n");

    // Acquire read lock
    // if(pthread_rwlock_rdlock(file_lock) != 0){
    //     send_error_to_client(nm_socket, "Failed to acquire read lock");
    //     return;
    // }

    // Original file handling code
    printf("path in read_file: %s\n", path);
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("ERROR opening file for reading");
        return;
    }

    SS_to_NM_struct data;
    data.is_directory = 0; // Signal this is a regular file
    data.stop = 0;
    // printf("sending data with stop = 0\n");
    // send(nm_socket, &data, sizeof(data), 0);

    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    bytes = read(fd, buffer, sizeof(buffer));
    buffer[bytes] = '\0';
    strcpy(data.buffer, buffer);
    printf("buffer read in nm_read_file %s\n", buffer);
    data.buffer_size = bytes;
    data.stop = 0;
    printf("sending data with stop = 0\n");

    // pthread_mutex_lock(&lock);
    if (send(nm_socket, &data, sizeof(SS_to_NM_struct), 0) < 0)
    {                                       
        perror("ERROR sending file data");
    }
    // pthread_mutex_unlock(&lock);
    printf("data.buffer in nm_read_file: %s\n", data.buffer);
    memset(buffer, 0, BUFFER_SIZE);

    close(fd);
    // pthread_rwlock_unlock(file_lock);
}

void *async_file_write_thread(void *arg)
{
    ThreadArgs *thread_args = (ThreadArgs *)arg;
    int client_socket = thread_args->client_socket;
    
    pthread_rwlock_t* file_lock = get_file_lock(thread_args->path);
    if(!file_lock){
        send_error_to_client(client_socket, "Failed to acquire file lock");
        return NULL;
    }

    // Acquire write lock
    if(pthread_rwlock_wrlock(file_lock) != 0){
        send_error_to_client(client_socket, "Failed to acquire write lock");
        return NULL;
    }
    int fd = open(thread_args->path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        pthread_rwlock_unlock(file_lock);
        perror("ERROR opening file for writing");
        send_error_to_client(client_socket, "Failed to open file");
        free(thread_args);
        return NULL;
    }

    // Trim newline if present
    size_t len = strlen(thread_args->buffer);
    if (len > 0 && thread_args->buffer[len-1] == '\n') {
        thread_args->buffer[len-1] = '\0';
        len--;
    }

    // pthread_mutex_lock(&lock);
    if (write(fd, thread_args->buffer, len) < 0)
    {
        perror("ERROR writing to file");
    }
    // pthread_mutex_unlock(&lock);
    close(thread_args->client_socket);
    close(fd);
    pthread_rwlock_unlock(file_lock);
    free(thread_args);
    return NULL;
}

void write_file(int client_socket, const char *path, int is_async)
{
    if (is_async)
    {
        // Send acknowledgement
        int ack = 1;

        // pthread_mutex_lock(&lock);
        send(client_socket, &ack, sizeof(ack), 0);
        // pthread_mutex_unlock(&lock);
        int bytes;
        pthread_t thread;
        ThreadArgs *thread_args = (ThreadArgs*)malloc(sizeof(ThreadArgs));
        bytes = recv(client_socket, thread_args->buffer, sizeof(thread_args->buffer), 0);
        // if (first_receive) {
        //     first_receive = 0;
        //     continue;
        // }
        // Create thread for async writing
        thread_args->client_socket = client_socket;
        strcpy(thread_args->path,path);

        if (pthread_create(&thread, NULL, async_file_write_thread, thread_args) != 0)
        {
            perror("ERROR creating thread");
            send_error_to_client(client_socket, "Failed to create async thread");
            free(thread_args);
            return;
        }
        pthread_detach(thread);
    }
    else
    {
        pthread_rwlock_t* file_lock = get_file_lock(path);
        if(!file_lock){
            send_error_to_client(client_socket, "Failed to acquire file lock");
            return;
        }

        // Acquire write lock
        if(pthread_rwlock_wrlock(file_lock) != 0){
            send_error_to_client(client_socket, "Failed to acquire write lock");
            return;
        }
        // Synchronous file writing
        int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0)
        {
            pthread_rwlock_unlock(file_lock);
            perror("ERROR opening file for writing");
            send_error_to_client(client_socket, "Failed to open file");
            return;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes;
        int first_receive = 1;

        // pthread_mutex_lock(&lock);
        bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        // pthread_mutex_unlock(&lock);
        printf("\n\n%s\n\n", buffer);
        // if (first_receive)
        // {
        //     first_receive = 0;
        //     continue;
        // }

        // pthread_mutex_lock(&lock);
        if (write(fd, buffer, strlen(buffer)) < 0)
        {
            perror("ERROR writing to file");
        }
        // pthread_mutex_unlock(&lock);
        close(client_socket);
        close(fd);
        pthread_rwlock_unlock(file_lock);
    }
}



void append_file(int nm_socket, const char *path)
{
    // pthread_rwlock_t* file_lock = get_file_lock(path);
    // if(!file_lock){
    //     send_error_to_client(nm_socket, "Failed to acquire append lock");
    //     return;
    // }

    // Acquire append lock
    // pthread_rwlock_wrlock(file_lock);
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);  // Added O_CREAT
    if (fd < 0)
    {
        // pthread_rwlock_unlock(file_lock);
        perror("ERROR opening file for writing");
        send_error_to_client(nm_socket, "Failed to open file");
        return;
    }
    
    SS_to_NM_struct data;
    // memset(data.buffer, 0, sizeof(data.buffer));
    ssize_t bytes;
    
    // while (strlen(data.buffer) == 0)
    // {  
    
    // pthread_mutex_lock(&lock);
    bytes = recv(nm_socket, &data, sizeof(data), MSG_WAITALL);
    // pthread_mutex_unlock(&lock);
    printf("data.stop: %d; bytes: %ld\n", data.stop, bytes);
    // if (data.stop == 1)
    //     break;s
    // }

    // pthread_mutex_lock(&lock);
    if (write(fd, data.buffer, strlen(data.buffer)) < 0)  // Write only the actual data size
    {
        perror("ERROR writing to file");
        // break;
    }
    // pthread_mutex_unlock(&lock);
    close(fd);
    // pthread_rwlock_unlock(file_lock);
}

void get_size_and_permissions(int client_socket, const char *path)
{
    struct stat file_stat;
    if (stat(path, &file_stat) < 0)
    {
        perror("ERROR getting file information");
        return;
    }

    // pthread_mutex_lock(&lock);
    send(client_socket, &file_stat, sizeof(file_stat), 0);
    // pthread_mutex_unlock(&lock);
}

void stream_audio_file(int client_socket, const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        perror("ERROR opening audio file for streaming");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes;
    while ((bytes = read(fd, buffer, sizeof(buffer))) > 0)
    {
        if (send(client_socket, buffer, bytes, 0) < 0)
        {
            perror("ERROR streaming audio to client");
            break;
        }
    }

    // pthread_mutex_lock(&lock);
    if (send(client_socket, "STOP", bytes, 0) < 0)
    {
        perror("ERROR streaming audio to client");
    }
    // pthread_mutex_unlock(&lock);
    close(fd);
}

void handle_client_request(int client_socket)
{
    client_to_NM command_buffer;
    ssize_t bytes_read = recv(client_socket, &command_buffer, sizeof(command_buffer), 0);

    if (command_buffer.oper == READ)
    {
        read_file(client_socket, command_buffer.file_path);
        close(client_socket);
    }
    else if (command_buffer.oper == WRITE)
    {
        write_file(client_socket, command_buffer.file_path, command_buffer.is_async);
        // if (!command_buffer.is_async)
        //     close(client_socket);
    }
    else if (command_buffer.oper == GETINFO)
    {
        get_size_and_permissions(client_socket, command_buffer.file_path);
        close(client_socket);
    }
    else if (command_buffer.oper == STREAM)
    {
        stream_audio_file(client_socket, command_buffer.file_path);
        close(client_socket);
    }
}

// Function to retrieve the actual IP address of the Storage Server
void get_ip_address(char *ip_buffer, size_t buffer_size)
{
    struct ifaddrs *ifaddr, *ifa;
    int family;

    if (getifaddrs(&ifaddr) == -1)
    {
        error("ERROR getting network interfaces");
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET)
        {
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ip_buffer, buffer_size, NULL, 0, NI_NUMERICHOST) == 0)
            {
                if (strcmp(ifa->ifa_name, "lo") != 0)
                { // Skip the loopback interface
                    break;
                }
            }
        }
    }

    freeifaddrs(ifaddr);
}

void recursive_path_search(const char *directory, char ***paths, int *path_count)
{
    struct dirent *entry;
    DIR *dp = opendir(directory);
    printf("Called!!\n");
    if (dp == NULL)
    {
        perror("ERROR opening directory");
        return;
    }

    while ((entry = readdir(dp)))
    {
        // Skip the current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        // Construct the full path
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, MAX_PATH_LENGTH + 1, "%s/%s", directory, entry->d_name);

        // Check if it's a directory
        struct stat entry_stat;
        if (stat(full_path, &entry_stat) == -1)
        {
            perror("ERROR getting file status");
            continue;
        }

        if (S_ISDIR(entry_stat.st_mode))
        {
            // If it's a directory, recursively search inside it
            recursive_path_search(full_path, paths, path_count);
        }
        else
        {
            // Otherwise, add the file path to the list
            *paths = realloc(*paths, sizeof(char *) * (*path_count + 1));
            if (*paths == NULL)
            {
                perror("ERROR reallocating memory");
                closedir(dp);
                return;
            }

            (*paths)[*path_count] = malloc(MAX_PATH_LENGTH);
            if ((*paths)[*path_count] == NULL)
            {
                perror("ERROR allocating memory");
                closedir(dp);
                return;
            }

            strncpy((*paths)[*path_count], full_path, MAX_PATH_LENGTH - 1);
            (*paths)[*path_count][MAX_PATH_LENGTH - 1] = '\0'; // Ensure null termination
            (*path_count)++;
        }
    }

    closedir(dp);
}

char **get_accessible_paths(int *path_count)
{
    *path_count = 0;
    char **paths = NULL;

    // Start the recursive search from the root directory
    recursive_path_search(storage_directory, &paths, path_count);

    return paths;
}

void initialize_storage_server(const char *nm_ip, int nm_port, int predefined_client_connection_port)
{
    struct sockaddr_in nm_addr, client_addr, alive_addr;
    char buffer[BUFFER_SIZE];
    ssize_t n;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Step 1: Create a socket to connect to NM
    nm_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (nm_sockfd < 0)
    {
        error("ERROR opening NM socket");
    }

    // Define the NM server address
    memset(&nm_addr, 0, sizeof(nm_addr));
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(nm_port);
    if (inet_pton(AF_INET, nm_ip, &nm_addr.sin_addr) <= 0)
    {
        error("ERROR invalid NM IP address");
    }

    // Connect to the Naming Server
    if (connect(nm_sockfd, (struct sockaddr *)&nm_addr, sizeof(nm_addr)) < 0)
    {
        error("ERROR connecting to Naming Server");
    }

    // Retrieve the NM connection port assigned to this socket
    struct sockaddr_in local_addr;
    if (getsockname(nm_sockfd, (struct sockaddr *)&local_addr, &addr_len) == -1)
    {
        error("ERROR getting NM connection port");
    }
    int nm_connection_port = ntohs(local_addr.sin_port);

    // Step 2: Send initialization signal (2) and wait for acknowledgment
    int init_buffer = 2;
    if (send(nm_sockfd, &init_buffer, sizeof(int), 0) < 0)
    {
        error("ERROR sending initialization signal");
    }


    // Step 3: Set up a socket to listen for client connections
    // Step 2: Set up a socket to listen for client connections
    client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd < 0)
    {
        error("ERROR opening client socket");
    }

    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;

    // If a predefined client connection port is provided, bind to that port
    if (predefined_client_connection_port > 0) {
        client_addr.sin_port = htons(predefined_client_connection_port);
        
        if (bind(client_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
        {
            error("ERROR binding to predefined client connection port");
        }
    }
    else {
        // If no predefined port, bind to any available port
        client_addr.sin_port = 0;
        if (bind(client_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0)
        {
            error("ERROR binding client socket");
        }
    }

    // Retrieve the assigned client connection port
    if (getsockname(client_sockfd, (struct sockaddr *)&client_addr, &addr_len) == -1)
    {
        error("ERROR getting client connection port");
    }
    int client_connection_port = ntohs(client_addr.sin_port);

    // Step 4: Set up a socket to listen for alive checker connections
    alive_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (alive_sockfd < 0)
    {
        error("ERROR opening alive checker socket");
    }

    memset(&alive_addr, 0, sizeof(alive_addr));
    alive_addr.sin_family = AF_INET;
    alive_addr.sin_addr.s_addr = INADDR_ANY;
    alive_addr.sin_port = 0;

    if (bind(alive_sockfd, (struct sockaddr *)&alive_addr, sizeof(alive_addr)) < 0)
    {
        error("ERROR binding alive checker socket");
    }

    // Retrieve the assigned alive checker port
    if (getsockname(alive_sockfd, (struct sockaddr *)&alive_addr, &addr_len) == -1)
    {
        error("ERROR getting alive checker port");
    }
    int alive_connection_port = ntohs(alive_addr.sin_port);

    // Step 5: Prepare the StorageServerInfo struct
    struct StorageServerInfo ss_info;
    get_ip_address(ss_info.ip, sizeof(ss_info.ip));
    ss_info.nm_connection_port = nm_connection_port;
    ss_info.client_connection_port = client_connection_port;
    ss_info.alive_connection_port = alive_connection_port; // NEW: Add alive checker port

    char **paths = get_accessible_paths(&ss_info.path_count);
    if (paths == NULL)
    {
        error("ERROR retrieving accessible paths");
    }

    for (int i = 0; i < ss_info.path_count; i++)
    {
        strncpy(ss_info.accessible_paths[i], paths[i], MAX_PATH_LENGTH - 1);
        ss_info.accessible_paths[i][MAX_PATH_LENGTH - 1] = '\0';
        printf("Accessible Path %d: %s\n", i + 1, ss_info.accessible_paths[i]);
        free(paths[i]);
    }
    free(paths);

    // Step 6: Send the struct to the Naming Server
    if (send(nm_sockfd, &ss_info, sizeof(ss_info), 0) < 0)
    {
        error("ERROR sending StorageServerInfo struct");
    }
    n = recv(nm_sockfd, &init_buffer, sizeof(int), 0);
    if (n < 0)
    {
        error("ERROR receiving acknowledgment");
    }
    char backupname[MAX_PATH_LENGTH];
    snprintf(backupname, sizeof(backupname), "backup_%d/", init_buffer);
    create_file_or_directory(backupname);

    printf("Storage Server initialized and registered with Naming Server.\n");
    printf("SS IP: %s, NM Port: %d, Client Port: %d, Alive Port: %d\n",
           ss_info.ip, ss_info.nm_connection_port, ss_info.client_connection_port,
           ss_info.alive_connection_port);

    // Start listening on both client and alive checker sockets
    if (listen(client_sockfd, 5) < 0)
    {
        error("ERROR on client listen");
    }
    if (listen(alive_sockfd, 5) < 0)
    {
        error("ERROR on alive checker listen");
    }

    printf("Storage Server is now listening for:\n");
    printf("- Client connections on port %d\n", client_connection_port);
    printf("- Alive checker connections on port %d\n", alive_connection_port);
}


void *handle_client(void *arg)
{
    client_info_t *client_info = (client_info_t *)arg;
    int new_client_sockfd = client_info->client_sockfd;
    struct sockaddr_in client_addr = client_info->client_addr;

    printf("Accepted connection from %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    handle_client_request(new_client_sockfd);
    // Close the client connection and free memory
    close(new_client_sockfd);
    free(client_info);
    return NULL;
}

// Thread that listens and accepts new connections
void *accept_client_connections(void *arg)
{
    int server_sockfd = *(int *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (1)
    {
        // Accept new connections
        int new_client_sockfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &addr_len);
        if (new_client_sockfd == -1)
        {
            perror("Accept failed");
            continue;
        }

        // Allocate memory for client info
        client_info_t *client_info = malloc(sizeof(client_info_t));
        if (client_info == NULL)
        {
            perror("Malloc failed");
            close(new_client_sockfd);
            continue;
        }
        client_info->client_sockfd = new_client_sockfd;
        client_info->client_addr = client_addr;

        // Create a thread to handle the client
        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client, client_info) != 0)
        {
            perror("Failed to create thread for client");
            free(client_info);
            close(new_client_sockfd);
            continue;
        }

        pthread_detach(client_thread); // Detach the thread to clean up resources automatically
    }

    return NULL;
}

void *accept_nm_connections(void *arg)
{

    client_to_NM nm_info;
    
    while (1)
    {
        int ack = 1;
        printf("FLow reaches here\n");
        // pthread_mutex_lock(&lock);
        int bytes_received = recv(nm_sockfd, &nm_info, sizeof(client_to_NM), MSG_WAITALL);
        // pthread_mutex_unlock(&lock);
        if (bytes_received <= 0)
        {
            printf("Breaking\n");
            exit(0);
        }

        printf("Recieved path: %s\n", nm_info.file_path);
        printf("Recieved command: %d\n", nm_info.oper);
        if (nm_info.oper == DELETE)
        {
            delete_file_or_directory(nm_info.file_path);
            send(nm_sockfd, &ack, sizeof(int), 0);
        }
        else if (nm_info.oper == CREATE)
        {
            printf("Calling CREATE in accept_nm_connections\nnm_info struct:\n");
            printf("nm_info.file_path: %s\n", nm_info.file_path);
            printf("nm_info.file_path_2: %s\n", nm_info.file_path_2);
            printf("nm_info.oper: %d\n", nm_info.oper);
            printf("nm_info.is_async: %d\n", nm_info.is_async);

            strcat(nm_info.file_path, nm_info.file_path_2);
            create_file_or_directory(nm_info.file_path);
            printf("Create returned\n");
        }
        else if (nm_info.oper == READ)
        {
            printf("Recieved\n");
            nm_read_file(nm_sockfd, nm_info.file_path);
        }
        else if (nm_info.oper == WRITE)
        {
            append_file(nm_sockfd, nm_info.file_path);
            send(nm_sockfd, &ack, sizeof(int), 0);
            printf("Sent ACK: 1 after killing raveesh (writing)\n");
        }
        // send(nm_sockfd, &ack, sizeof(int), 0);
        printf("Some op was performed by NM\n");
    }
    return NULL;
}

// Helper function to get port number from socket
int get_port(int sockfd)
{
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    if (getsockname(sockfd, (struct sockaddr *)&addr, &addr_len) < 0)
    {
        perror("ERROR getting socket port");
        return -1;
    }

    return ntohs(addr.sin_port);
}

void *alive_checker_connection(void *arg)
{
    (void)arg; // Unused parameter

    struct sockaddr_in checker_addr;
    socklen_t addr_len = sizeof(checker_addr);

    printf("Starting alive checker thread on port %d\n",
           get_port(alive_sockfd)); // Use the port we set up in initialize_storage_server

    while (1)
    {
        // Accept connection from the alive checker
        int checker_conn = accept(alive_sockfd,
                                  (struct sockaddr *)&checker_addr,
                                  &addr_len);

        if (checker_conn < 0)
        {
            perror("ERROR accepting alive checker connection");
            continue; // Continue listening even if one accept fails
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(checker_addr.sin_addr),
                  client_ip, INET_ADDRSTRLEN);

        printf("Alive check received from %s:%d\n",
               client_ip,
               ntohs(checker_addr.sin_port));

        // Close the connection immediately as per the reference implementation
        if (close(checker_conn) < 0)
        {
            perror("ERROR closing alive checker connection");
        }
    }

    // This part will never be reached in normal operation
    // but included for completeness
    if (close(alive_sockfd) < 0)
    {
        perror("ERROR closing alive checker socket");
    }

    return NULL;
}

void init_lock_manager(){
    memset(&lock_manager, 0, sizeof(file_lock_manager_t));
    pthread_mutex_init(&lock_manager.lock_array_mutex, NULL);
    for(int i = 0; i < MAX_FILES; i++){
        lock_manager.locks[i].is_initialized = 0;
    }
}

pthread_rwlock_t* get_file_lock(const char* filepath) {
    pthread_mutex_lock(&lock_manager.lock_array_mutex);
    
    // First, check if lock already_lock exists
    for (int i = 0; i < lock_manager.count; i++) {
        if (strcmp(lock_manager.locks[i].filepath, filepath) == 0) {
            pthread_mutex_unlock(&lock_manager.lock_array_mutex);
            return &lock_manager.locks[i].lock;
        }
    }
    
    // If lock doesn't exist and we have space, create new lock
    if (lock_manager.count < MAX_FILES) {
        int idx = lock_manager.count;
        strncpy(lock_manager.locks[idx].filepath, filepath, PATH_MAX - 1);
        lock_manager.locks[idx].filepath[PATH_MAX - 1] = '\0';
        
        if (!lock_manager.locks[idx].is_initialized) {
            pthread_rwlock_init(&lock_manager.locks[idx].lock, NULL);
            lock_manager.locks[idx].is_initialized = 1;
        }
        
        lock_manager.count++;
        pthread_mutex_unlock(&lock_manager.lock_array_mutex);
        return &lock_manager.locks[idx].lock;
    }
    
    pthread_mutex_unlock(&lock_manager.lock_array_mutex);
    return NULL;
}

void cleanup_lock_manager() {
    pthread_mutex_lock(&lock_manager.lock_array_mutex);
    
    for (int i = 0; i < lock_manager.count; i++) {
        if (lock_manager.locks[i].is_initialized) {
            pthread_rwlock_destroy(&lock_manager.locks[i].lock);
            lock_manager.locks[i].is_initialized = 0;
        }
    }
    
    pthread_mutex_unlock(&lock_manager.lock_array_mutex);
    pthread_mutex_destroy(&lock_manager.lock_array_mutex);
}