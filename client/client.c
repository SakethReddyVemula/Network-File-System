#include"client.h"

void print_file_info(const struct stat *file_stat) {
    // File size
    printf("File size: %lld bytes\n", (long long) file_stat->st_size);

    // File permissions
    printf("Access rights: ");
    printf((S_ISDIR(file_stat->st_mode)) ? "d" : "-");
    printf((file_stat->st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat->st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat->st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat->st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat->st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat->st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat->st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat->st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat->st_mode & S_IXOTH) ? "x" : "-");
    printf("\n");

    // Last access time
    printf("Last access time: %s", ctime(&file_stat->st_atime));

    // Last modification time
    printf("Last modification time: %s", ctime(&file_stat->st_mtime));

    // Last status change time
    printf("Last status change time: %s", ctime(&file_stat->st_ctime));

    // Additional metadata
    printf("Device ID: %lld\n", (long long) file_stat->st_dev);
    printf("Inode number: %lld\n", (long long) file_stat->st_ino);
    printf("Number of hard links: %lld\n", (long long) file_stat->st_nlink);
    printf("Owner user ID: %d\n", file_stat->st_uid);
    printf("Owner group ID: %d\n", file_stat->st_gid);
}

// Sets up sock by connecting to the provided server ip address and port number
int connect_to_server(char* name_server_ip, int name_server_port){
    int sock;
    struct sockaddr_in server_addr;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket");
        exit(1);
    }
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(name_server_port);

    if (inet_pton(AF_INET, name_server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect");
        exit(1);
    }

    return sock;
}

// Tokenizes a string by space, tab and newline. Returns the number of tokens
int tokenize(char* buffer, char** tokens){
    int num_tokens = 0;
    tokens[num_tokens] = strtok(buffer, " \n\t");
    while (tokens[num_tokens] != NULL){
        num_tokens += 1;
        tokens[num_tokens] = strtok(NULL, " \n\t");
    }
    return num_tokens;

}

// Accepts acknowledgement of connection from NM. Returns 0 if success, and 1 if failure
int accept_connection_acknowledgement(int sock){
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sock, &readfds);
    int res = select(sock + 1, &readfds, NULL, NULL, &timeout); // Use select to wait until the socket is ready to receive, or time runs out
    if (res == -1){
        perror("Select");
        return 1;
    }
    else if (res == 0){
        printf("Request timed out\n");
        return 1;
    }
    int rec;
    recv(sock, &rec, sizeof(int), 0); // The name server should return an acknowledgement with a single integer 1 upon client connection.
    if (rec == 1){
        return 0;
    }
}