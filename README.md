# Operating Systems and Networks
## Course Project - Network File System (NFS) 
### Team - 2: 
#### Raveesh (2022114002)
#### Saketh (2022114014)
#### Abraham Paul (2022114007)
#### Vivek Hruday (2022114012)
---


### Assumptions
1. CREATE command cannot create any directories at level zero, it at least has to specify one directory in which it wants to create a file or directory.
2. Do not input an initial slash in the path.
3. When we are sending a folder it must end with a slash.
4. We assume that none of the files or folders in a storage server are empty.
5. When a Storage Server goes offline, we will wait 15 seconds to issue redundancy commands.
6. Port number is constant for different storage servers.

### File structure
`client/` contains all the files related to clients:
- `client.h` contains all the declarations of the functions and global variables used.
- `client.c` contains all the functions and their implementations used.
- `main.c` contains implementation of the main function and some of the helper functions.

`common/` contains only one file `common.h` which has declarations and global variables that are used commonly across all three components.

`naming_server/` contains all the files related to the naming server:
- `lru.c` has implementations of the LRU cache.
- `online.c` contains implementations for checking if all the storage servers are online periodically.
-  `trie.c` contains implementation of the trie for efficient searching and also some of the helper functions.
- `naming_server.c` contains implementations of the functions used across the naming server.
- `naming_server.h` holds the function definitions of the functions in this folder.
- `main.c` contains the main function of this modular code.

`storage_server/` contains all the files related to storage server:
- `storage_server.c`contains all the implementations of the functions used across this folder.
- `storage_server.h` contains the declarations of the functions that are implemented in storage_server.c.
- `main.c` contains the main function of this modular code.

### To run the code
Open different terminals for different servers and clients.

For a naming server, run:

```bash
cd ./naming_server
gcc *.c
./a.out
```
The above prints out the IP address and different port numbers for client and storage server

For a client, run:

```bash
cd ./client
gcc *.c
./a.out <Naming Server IP> <Naming Server Port>
```

For a storage_server, run:

```bash
cd ./storage_server
gcc *.c
./a.out <Naming Server IP> <Naming Server Port> <Path to directory of storage server>
```
