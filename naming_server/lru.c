#include"naming_server.h"


// Hash function
int hash(const char* key) {
    int hash = 0;
    while (*key) hash = (hash + *key++) % CACHE_CAPACITY;
    return hash;
}

// Create a new cache node
CacheNode* createNode(const char* key, int value) {
    CacheNode* node = (CacheNode*)malloc(sizeof(CacheNode));
    strcpy(node->key, key);
    node->value = value;
    node->prev = node->next = NULL;
    return node;
}

// Initialize the LRU cache
LRUCache* createCache() {
    LRUCache* cache = (LRUCache*)malloc(sizeof(LRUCache));
    cache->head = cache->tail = NULL;
    cache->size = 0;
    memset(cache->hashmap, 0, sizeof(cache->hashmap));
    return cache;
}

// Move a node to the front
void moveToHead(LRUCache* cache, CacheNode* node) {
    if (cache->head == node) return;

    // Remove node from its current position
    if (node->prev) node->prev->next = node->next;
    if (node->next) node->next->prev = node->prev;
    if (node == cache->tail) cache->tail = node->prev;

    // Move to the head
    node->prev = NULL;
    node->next = cache->head;
    if (cache->head) cache->head->prev = node;
    cache->head = node;

    if (cache->tail == NULL) cache->tail = node;
}

// Remove the least recently used (LRU) node
void removeLRU(LRUCache* cache) {
    if (cache->tail == NULL) return;

    CacheNode* lru = cache->tail;
    int index = hash(lru->key);

    // Update hashmap
    cache->hashmap[index] = NULL;

    // Remove from the linked list
    if (lru->prev) lru->prev->next = NULL;
    cache->tail = lru->prev;
    if (cache->tail == NULL) cache->head = NULL;

    free(lru);
    cache->size--;
}

// Add or update a key-value pair
void put(LRUCache* cache, const char* key, int value) {
    int index = hash(key);

    CacheNode* node = cache->hashmap[index];

    // Check if the key exists
    while (node && strcmp(node->key, key) != 0) {
        node = node->next;
    }

    if (node) {
        // Update value and move to head
        node->value = value;
        moveToHead(cache, node);
    } else {
        // Create a new node
        CacheNode* newNode = createNode(key, value);

        // Add to hashmap
        newNode->next = cache->hashmap[index];
        cache->hashmap[index] = newNode;

        // Add to the front of the list
        moveToHead(cache, newNode);

        cache->size++;
        if (cache->size > CACHE_CAPACITY) {
            removeLRU(cache);
        }
    }
}

// Get a value by key
int get(LRUCache* cache, const char* key) {
    int index = hash(key);

    CacheNode* node = cache->hashmap[index];

    // Find the node in the linked list
    while (node && strcmp(node->key, key) != 0) {
        node = node->next;
    }

    if (node) {
        // Move to head and return value
        moveToHead(cache, node);
        return node->value;
    }

    return -1; // Key not found
}

// Free the cache
void freeCache(LRUCache* cache) {
    CacheNode* current = cache->head;
    while (current) {
        CacheNode* temp = current;
        current = current->next;
        free(temp);
    }
    free(cache);
}

void removeFromCache(LRUCache* cache, const char* key) {
    int index = hash(key);

    CacheNode* node = cache->hashmap[index];
    CacheNode* prev = NULL;

    // Find the node in the linked list at the hashed index
    while (node && strcmp(node->key, key) != 0) {
        prev = node;
        node = node->next;
    }

    if (!node) {
        // Key not found in the cache
        printf("Key %s not found in cache\n", key);
        return;
    }

    // Remove from hashmap
    if (prev) {
        prev->next = node->next;
    } else {
        cache->hashmap[index] = node->next;
    }

    // Remove from the linked list
    if (node->prev) {
        node->prev->next = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    }
    if (node == cache->head) {
        cache->head = node->next;
    }
    if (node == cache->tail) {
        cache->tail = node->prev;
    }

    // Free the memory and update cache size
    free(node);
    cache->size--;

    printf("Key %s removed from cache\n", key);
}
