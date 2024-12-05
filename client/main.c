#include "client.h"

// Command handler function prototypes
typedef int (*command_handler)(int sock_NM, const char *file_path);

// Structure to map commands to their handlers
typedef struct
{
    const char *command;
    command_handler handler;
} command_mapping;


void check_socket_status(int sockfd) {
    int bytes_available = 0;

    // Query the number of bytes available to read
    if (ioctl(sockfd, FIONREAD, &bytes_available) < 0) {
        perror("ioctl failed");
        return;
    }

    if (bytes_available > 0) {
        printf("Socket has %d bytes available to read.\n", bytes_available);
    } else {
        printf("Socket has no data available to read.\n");
    }
}

int handle_read(int sock_NM, const char *file_path)
{
    client_to_NM send_client_to_nm = {.oper = READ};
    NM_to_client recv_nm_to_client;

    strcpy(send_client_to_nm.file_path, file_path);
    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);

    if (recv(sock_NM, &recv_nm_to_client, sizeof(recv_nm_to_client), 0) == -1)
    {
        perror("recv nm_to_client");
        return 1;
    }

    if (recv_nm_to_client.e_code == SUCCESS)
    {
        int sock_SS = connect_to_server(recv_nm_to_client.IP_SS, recv_nm_to_client.port_SS);
        if (sock_SS < 0)
        {
            perror("Failed to connect to SS");
            return 1;
        }

        send(sock_SS, &send_client_to_nm, sizeof(send_client_to_nm), 0);

        char buffer[BUFFER_SIZE];
        ssize_t bytes_received;
        // Clear the buffer before each read
        memset(buffer, 0, BUFFER_SIZE);

        while ((bytes_received = recv(sock_SS, buffer, BUFFER_SIZE - 1, 0)) > 0)
        {
            // Ensure null termination
            buffer[bytes_received] = '\0';
            printf("%s", buffer);
            // Clear the buffer before next read
            memset(buffer, 0, BUFFER_SIZE);
        }

        close(sock_SS);
        return 0;
    }
    else if (recv_nm_to_client.e_code == MAIN_SERVER_DOWN){
        SS_to_NM_struct data;
        // if (recv(sock_NM, &data, sizeof(data), 0) <= 0) {
        //     perror("ERROR receiving initial data");
        //     return -1;
        // }
        // while (1) {
        //     if (recv(sock_NM, &data, sizeof(data), 0) <= 0) {
        //         perror("ERROR receiving file data");
        //         break;
        //     }
            
        //     if (data.stop) {
        //         break;
        //     }
            
        //     printf("%s", data.buffer);
        // }
        if (recv(sock_NM, &data, sizeof(data), MSG_WAITALL) <= 0) {
            perror("ERROR receiving file data");
        }
        
        printf("%s\n", data.buffer);
    }
    else{
        printf("Error: Unable to access file at path %s\n", file_path);
    }
    return 1;
    

}

void *async_send_thread(void *arg)
{
    async_write_args *write_info = (async_write_args *)arg;
    send(write_info->sock_SS, write_info->buffer, sizeof(write_info->buffer), 0);
    printf("Sent sent sent\n");
    // close(write_info->sock_SS);
    free(write_info);
    return NULL;
}

int handle_write(int sock_NM, const char *file_path, int is_async)
{
    client_to_NM send_client_to_nm = {.oper = WRITE};
    NM_to_client recv_nm_to_client;
    printf("Reached here\n");
    strcpy(send_client_to_nm.file_path, file_path);
    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);
    if (recv(sock_NM, &recv_nm_to_client, sizeof(recv_nm_to_client), 0) == -1)
    {
        perror("recv nm_to_client");
        return 1;
    }

    printf("Server IP: %s\n", recv_nm_to_client.IP_SS);
    if (recv_nm_to_client.e_code == SUCCESS)
    {
        int sock_SS = connect_to_server(recv_nm_to_client.IP_SS, recv_nm_to_client.port_SS);
        if (sock_SS < 0)
        {
            perror("Failed to connect to Storage Server");
            return 1;
        }
        // printf("%d\n", sock_SS);
        if (is_async)
        {
            // Prepare buffer
            // Create thread for async send

            client_to_NM send_client_to_nm = {.oper = WRITE};
            strcpy(send_client_to_nm.file_path, file_path);
            send_client_to_nm.is_async = 1;

            send(sock_SS, &send_client_to_nm, sizeof(send_client_to_nm), 0);
            int ack_async = -1;
            char buffer[BUFFER_SIZE] = {0};
            printf("Enter data to write: \n");
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
            {
                perror("Error reading input");
                return 1;
            }
            // Remove newline if present
            size_t len = strlen(buffer);
            recv(sock_SS, &ack_async, sizeof(int), 0);
            printf("%d\n", ack_async);
            if (ack_async == 1)
                printf("Success\n");
            else
                return 1;
            printf("Async write request acknowledement recieved\n");
            async_write_args *write_info = malloc(sizeof(async_write_args));
            write_info->sock_SS = sock_SS;
            write_info->file_path = file_path;
            strcpy(write_info->buffer, buffer);
            pthread_t send_thread;
            pthread_create(&send_thread, NULL, async_send_thread, write_info);
            pthread_detach(send_thread);

            return 0;
        }
        else
        {
            client_to_NM send_client_to_nm = {.oper = WRITE};
            strcpy(send_client_to_nm.file_path, file_path);
            char buffer[BUFFER_SIZE] = {0};
            printf("Enter data to write: \n");
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
            {
                perror("Error reading input");
                return 1;
            }

            // Remove newline if present
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len - 1] == '\n')
            {
                buffer[len - 1] = '\0';
                len--;
            }
            printf("%s\n", buffer);
            send_client_to_nm.is_async = 0;
            send(sock_SS, &send_client_to_nm, sizeof(send_client_to_nm), 0);
            send(sock_SS, buffer, sizeof(buffer), 0);
            close(sock_SS);
            return 0;
        }
    }

    printf("Error: Unable to write to file at path %s\n", file_path);
    return 1;
}

int handle_list(int sock_NM, const char *file_path)
{
    client_to_NM send_client_to_nm = {.oper = LIST};
    strcpy(send_client_to_nm.file_path, file_path);

    printf("For LIST path is:%s\n", send_client_to_nm.file_path);

    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);

    printf("Accessible Paths:\n");
    int bytes_received;
    char buffer[BUFFER_SIZE];
    // char output[100000];
    // while ((bytes_received = recv(sock_NM, buffer, BUFFER_SIZE - 1, 0)) > 0) 
    //     {// Connection closed or error
    //     buffer[bytes_received] = '\0'; // Null-terminate the received data
    //     printf("%s\n", buffer); // Print the valid received data
    //     if (strstr(buffer, "\n\n\n")) break; // Check for the stop signal
    // }

    while ((bytes_received = recv(sock_NM, buffer, BUFFER_SIZE, 0)) > 0) {
        // Null-terminate the received data
        buffer[bytes_received] = '\0';
        
        // Find the first occurrence of '/'
        char *slash_position = strchr(buffer, '/');
        
        // If '/' exists, print from there; otherwise, print the whole buffer
        if (slash_position) {
            printf("%s\n", slash_position);
        } else {
            // printf("%s\n", buffer);
        }

        // Check for the stop signal
        if (strstr(buffer, "\n\n\n")) break;
    }


    // printf("Done!\n");
    

    NM_to_client nm_to_client_t;
    // printf("Size of nm_to_client %lu\n", sizeof(nm_to_client_t));
    if (recv(sock_NM, &nm_to_client_t, sizeof(nm_to_client_t), 0) == -1) {
        perror("recv ack");
        return 1;
    }

    printf("Received ack\n");

    if (nm_to_client_t.e_code == INVALID_PATH) {
        printf("Invalid Path\n");
        printf("Error code: %d\n", nm_to_client_t.e_code);
        return 1;
    } 

    printf("Checking socket status\n");

    check_socket_status(sock_NM);


    return 0;
}



int handle_getinfo(int sock_NM, const char *file_path)
{
    client_to_NM send_client_to_nm = {.oper = GETINFO};
    NM_to_client recv_nm_to_client;

    strcpy(send_client_to_nm.file_path, file_path);
    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);

    if (recv(sock_NM, &recv_nm_to_client, sizeof(recv_nm_to_client), 0) == -1)
    {
        perror("recv nm_to_client");
        return 1;
    }

    if (recv_nm_to_client.e_code == SUCCESS)
    {
        int sock_SS = connect_to_server(recv_nm_to_client.IP_SS, recv_nm_to_client.port_SS);
        if (sock_SS < 0)
        {
            perror("Failed to connect to Storage Server");
            return 1;
        }

        send(sock_SS, &send_client_to_nm, sizeof(send_client_to_nm), 0);

        struct stat file_stat;
        if (recv(sock_SS, &file_stat, sizeof(file_stat), 0) > 0)
        {
            print_file_info(&file_stat);
        }
        close(sock_SS);
        return 0;
    }

    printf("Error: Unable to retrieve information for file at path %s\n", file_path);
    return 1;
}

int handle_stream(int sock_NM, const char *file_path)
{
    client_to_NM send_client_to_nm = {.oper = STREAM};
    NM_to_client recv_nm_to_client;

    strcpy(send_client_to_nm.file_path, file_path);
    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);

    if (recv(sock_NM, &recv_nm_to_client, sizeof(recv_nm_to_client), 0) == -1)
    {
        perror("recv nm_to_client");
        return 1;
    }

    if (recv_nm_to_client.e_code == SUCCESS)
    {
        int sock_SS = connect_to_server(recv_nm_to_client.IP_SS, recv_nm_to_client.port_SS);
        if (sock_SS < 0)
        {
            perror("Failed to connect to Storage Server");
            return 1;
        }

        send(sock_SS, &send_client_to_nm, sizeof(send_client_to_nm), 0);

        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            perror("pipe");
            close(sock_SS);
            return 1;
        }

        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            close(sock_SS);
            close(pipefd[0]);
            close(pipefd[1]);
            return 1;
        }

        if (pid == 0)
        { // Child process - runs mpv
            close(pipefd[1]);

            if (dup2(pipefd[0], STDIN_FILENO) == -1)
            {
                perror("dup2");
                exit(1);
            }
            close(pipefd[0]);

            execlp("mpv", "mpv", "-", NULL);
            perror("execlp");
            exit(1);
        }
        else
        { // Parent process - handles data from SS
            close(pipefd[0]);

            char buffer[BUFFER_SIZE];
            ssize_t bytes_received;

            // Clear buffer before reading
            memset(buffer, 0, BUFFER_SIZE);

            while ((bytes_received = recv(sock_SS, buffer, BUFFER_SIZE - 1, 0)) > 0)
            {
                if (strcmp(buffer, "STOP") == 0)
                {
                    break;
                }

                if (write(pipefd[1], buffer, bytes_received) != bytes_received)
                {
                    perror("write to pipe");
                    break;
                }
                // Clear buffer before next read
                memset(buffer, 0, BUFFER_SIZE);
            }

            close(pipefd[1]);
            int status;
            waitpid(pid, &status, 0);
        }

        close(sock_SS);
        return 0;
    }

    printf("Error: Unable to stream file at path %s\n", file_path);
    return 1;
}

// Simple command handlers for operations that only need to send command to NM
int handle_simple_command(int sock_NM, const char *file_path, operation oper)
{
    client_to_NM send_client_to_nm = {.oper = oper};
    strcpy(send_client_to_nm.file_path, file_path);
    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);
    return 0;
}

int handle_delete(int sock_NM, const char *file_path)
{
    printf("hello from handle_delete in client\n");
    client_to_NM send_client_to_nm = {.oper = DELETE};
    strcpy(send_client_to_nm.file_path, file_path);
    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);

    int ack;
    recv(sock_NM, &ack, sizeof(ack), 0);
    if (ack != 1)
    {
        printf("Error: ack\n");
        return 1;
    }
    printf("ACK=1 recieved\n");
    return 0;
}

int handle_create(int sock_NM, const char *file_path, const char *file_name)
{
    client_to_NM send_client_to_nm = {.oper = CREATE};
    strcpy(send_client_to_nm.file_path, file_path);
    strcpy(send_client_to_nm.file_path_2, file_name);
    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);

    NM_to_client nm_to_client_t;
    if (recv(sock_NM, &nm_to_client_t, sizeof(nm_to_client_t), 0) == -1)
    {
        perror("recv ack");
        return 1;
    }
    if (nm_to_client_t.ack == 1)
    {
        printf("File '%s' created successfully.\n", file_path);
        return 0;
    }
    else
    {
        printf("Error creating file '%s'.\n", file_path);
        printf("Error code: %d\n", nm_to_client_t.e_code);
        return 1;
    }
}

int handle_copy(int sock_NM, const char *src_file_path, const char *dest_file_path)
{
    client_to_NM send_client_to_nm = {.oper = COPY};
    strcpy(send_client_to_nm.file_path, src_file_path);
    strcpy(send_client_to_nm.file_path_2, dest_file_path);
    send(sock_NM, &send_client_to_nm, sizeof(send_client_to_nm), 0);

    NM_to_client nm_to_client_t;
    if (recv(sock_NM, &nm_to_client_t, sizeof(nm_to_client_t), 0) == -1)
    {
        perror("recv ack");
        return 1;
    }
    if (nm_to_client_t.ack == 1)
    {
        printf("File '%s' copied successfully to '%s'.\n", src_file_path, dest_file_path);
        printf("Error code: %d\n", nm_to_client_t.e_code);
        return 0;
    }
    else
    {
        printf("Error copying file '%s'.\n", dest_file_path);
        printf("Error code: %d\n", nm_to_client_t.e_code);
        return 1;
    }
}

// Command mapping table
const command_mapping COMMAND_TABLE[] = {
    {"READ", handle_read},
    // {"WRITE", handle_write},
    {"DELETE", handle_delete},
    // {"CREATE", handle_create},    // {"CREATE", handle_create},
    // {"COPY", handle_copy},
    {"LIST", handle_list},
    {"GETINFO", handle_getinfo},
    {"STREAM", handle_stream},
    {NULL, NULL} // Sentinel value
};

// Command dispatcher
int dispatch_command(const char *command, int sock_NM, const char *file_path)
{
    for (const command_mapping *cmd = COMMAND_TABLE; cmd->command != NULL; cmd++)
    {
        if (strcmp(command, cmd->command) == 0)
        {
            return cmd->handler(sock_NM, file_path);
        }
    }
    printf("Invalid command: %s\n", command);
    return 1;
}

int dispatch_create_command(const char *command, int sock_NM, const char *file_path, const char *file_name)
{
    handle_create(sock_NM, file_path, file_name);
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Enter IP and port of the name server\n");
        return 1;
    }

    int sock_NM = connect_to_server(argv[1], atoi(argv[2]));
    if (sock_NM < 0)
    {
        perror("Failed to connect to name server");
        return 1;
    }

    // Initialize connection with name server
    int clientInit = 1;
    send(sock_NM, &clientInit, sizeof(int), 0);
    if (accept_connection_acknowledgement(sock_NM) == 1)
    {
        return 1;
    }
    printf("Connected to server at IP: %s and Port: %s\n", argv[1], argv[2]);

    // Allocate memory for command processing
    char file_path[MAX_STRING_SIZE];
    char *tokens[MAX_TOKENS];
    for (int i = 0; i < MAX_TOKENS; i++)
    {
        tokens[i] = malloc(sizeof(char) * MAX_STRING_SIZE);
        if (!tokens[i])
        {
            perror("Failed to allocate memory for tokens");
            return 1;
        }
    }

    // Main command loop
    while (1)
    {
        if (!fgets(file_path, MAX_STRING_SIZE, stdin))
        {
            break;
        }

        int num_tokens = tokenize(file_path, tokens);
        if (num_tokens == 0)
            continue;

        if (strcmp(tokens[0], "EXIT") == 0)
        {
            break;
        }
        if(strcmp(tokens[0], "CREATE") && strcmp(tokens[0], "COPY") && strcmp(tokens[0], "LIST") && strcmp(tokens[0], "WRITE")){
            dispatch_command(tokens[0], sock_NM, tokens[1]);
        }
        else if (!strcmp(tokens[0], "CREATE"))
        {
            dispatch_create_command(tokens[0], sock_NM, tokens[1], tokens[2]);
        }
        else if (!strcmp(tokens[0], "COPY"))
        {
            handle_copy(sock_NM, tokens[1], tokens[2]);
        }
        else if(!strcmp(tokens[0], "LIST")){
            // printf("List enc\n");
            if (num_tokens == 1){
                handle_list(sock_NM, "/");
            } else {
                handle_list(sock_NM, tokens[1]);
            }
        }
        else if (!strcmp(tokens[0], "WRITE"))
        {
            if (num_tokens == 3 && !strcmp(tokens[2], "--async"))
                handle_write(sock_NM, tokens[1], 1);
            else
                handle_write(sock_NM, tokens[1], 0);
        }
    }

    // Cleanup
    // for (int i = 0; i < MAX_TOKENS; i++) {
    //     free(tokens[i]);
    // }
    close(sock_NM);
    return 0;
}