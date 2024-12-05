#include "storage_server.h"

const char *storage_directory;

// int main(int argc, char *argv[]) {
//     if (argc != 4) {
//         fprintf(stderr, "Usage: %s <NM_IP> <NM_Port> <Directory>\n", argv[0]);
//         exit(1);
//     }

//     const char *nm_ip = argv[1];
//     int nm_port = atoi(argv[2]);
//     storage_directory = argv[3];
    
//     init_lock_manager();
//     initialize_storage_server(nm_ip, nm_port);

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: %s <NM_IP> <NM_Port> <Directory> [NM_Connection_Port]\n", argv[0]);
        exit(1);
    }

    const char *nm_ip = argv[1];
    int nm_port = atoi(argv[2]);
    storage_directory = argv[3];
    
    // Optional predefined NM connection port
    int predefined_nm_connection_port = -1;
    if (argc == 5) {
        predefined_nm_connection_port = atoi(argv[4]);
    }
    
    init_lock_manager();
    initialize_storage_server(nm_ip, nm_port, predefined_nm_connection_port);
    
    pthread_t client_accept_thread;
    if (pthread_create(&client_accept_thread, NULL, accept_client_connections, &client_sockfd) != 0) {
        perror("Failed to create accept thread for clients");
        close(client_sockfd);
        return 1;
    }
    pthread_t nm_accept_thread;
    if (pthread_create(&nm_accept_thread, NULL, accept_nm_connections, NULL) != 0) {
        perror("Failed to create accept thread for Naming Server");
        close(nm_sockfd);
        return 1;
    }

    pthread_t alive_checker_thread;
    if(pthread_create(&alive_checker_thread, NULL, alive_checker_connection, NULL) != 0){
        perror("Failed to create alive checker thread for Naming Server");
        return 1;
    }

    atexit(cleanup_lock_manager); // atextit: registers a function to be called when "exit" is called;

    pthread_join(client_accept_thread, NULL);
    pthread_join(nm_accept_thread, NULL);
    pthread_join(alive_checker_thread, NULL);
    return 0;
}
