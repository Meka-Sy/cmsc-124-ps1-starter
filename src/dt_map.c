/*
 * dt_map.c: Associative arrays for Unit 5, Section E.
 *
 * An array does not store its indices. This map stores its keys.
 * An array calculates a position with one subtraction.
 * The map calculates a hash and then compares keys in one bucket.
 *
 * Hashing turns the key into a bucket number. Compare all keys in that bucket
 * because two keys can select it. Use a linked list for each bucket. Start the
 * unsigned accumulator at 14695981039346656037ULL. For each unsigned byte,
 * exclusive-or the byte into it and multiply by 1099511628211ULL.
 *
 * A separate list stores insertion order for stable output. dt_map_key_at
 * reads this list. Updating a key preserves its position. Removing and
 * reinserting a key moves it to the end.
 */

#include "dt.h"

#include <stdlib.h>
#include <string.h>

struct dt_map {
    // int placeholder; /* TODO: Add the buckets and insertion-order data. */
    // The map stores the keys, their values, and the links between entries.
    char **keys;
    dt_value *values;
    int *next;
    int *buckets;               // buckets stores which entry(key) belongs to each bucket
    char **order;               // order keeps the keys in the order they were inserted
    size_t capacity;            // number of entries the allocated array can hold
    size_t len;                 // number of entries currently in the map
    size_t bucket_count;        // number of buckets (in this program, it is specified as 5 in *dt_map_new())

};

/*
 * dt_map_new builds an empty map. It returns NULL after an allocation failure.
 */
dt_map *dt_map_new(void)
{
    /* TODO: Return an allocated empty map. Return NULL after an allocation failure.
       dt_map_new()  -> a map whose dt_map_len is 0
       cases/normal/map_basics.case */
    dt_map *m = calloc(1, sizeof(struct dt_map));               // Allocates space for map and initializes values to 0

    if (m == NULL) {                                            // if allocation fails
        return NULL;
    }

    // Sets the value of bucket_count to 5
    // Allocaties an integer size for each of the 5 buckets
    m->bucket_count = 5;
    m->buckets = malloc(m->bucket_count *sizeof(*m->buckets));

    // If the allocation fails, the allocated map needs to be freed 
    // or else the memory will remain allocated
    if(m->buckets == NULL){
        free(m);
        return NULL;
    }
    // Sets the value of each bucket to -1, which means that 
    // no entry(keys) are stored under any of the 5 buckets
    for (size_t i=0; i<m->bucket_count; i++){
        m->buckets[i] = -1;
    }
    return m;
    // return NULL;
}

/*
 * dt_map_free releases each entry, copied key, order array, and map.
 * It accepts NULL. The environment owns the values.
 */
void dt_map_free(dt_map *m)
{
    /* TODO: Release each entry, copied key, order array, and map.
       Preserve the values. The environment owns them.
       a map holding a string value  -> the nodes and keys go, the string stays
       dt_map_free(NULL)             -> returns, having done nothing
       cases/cleanup/map_churn.case */

    // Function returns there is no map to free
    if (m==NULL){
        return;
    }

    // The map copied each key, so it is responsible for freeing it. 
    for (size_t i=0; i<m->len; i++){
        free(m->keys[i]);
    }

    // The map also frees these storage arrays. 
    free(m->keys);
    free(m->values);
    free(m->next);
    free(m->buckets);
    free(m->order);
    free(m);                // Lastly, the map is freed.

    // (void)m;
}

/*
 * dt_map_len returns the number of keys in constant time.
 */
size_t dt_map_len(const dt_map *m)
{
    /* TODO: Return the current key count.
       Replacing a value does not change this count.
       after put alpha, beta, gamma:  dt_map_len(m) -> 3
       after put beta again:          dt_map_len(m) -> 3, still
       after del alpha:               dt_map_len(m) -> 2
       cases/normal/map_basics.case */
    if (m==NULL){           // if there is no map
        return 0;
    }
    return m->len;          // len has the number of entries(keys)
    // (void)m;
    // return 0;
}

/*
 * dt_map_put binds v to key.
 * An existing key keeps its insertion position. A new key becomes the last key.
 * Copy each new key because the caller owns the source buffer.
 * Return DT_ERR_CAPACITY after an allocation failure.
 */
dt_status dt_map_put(dt_map *m, const char *key, dt_value v)
{
    /* TODO: Replace the value for an existing key.
       Add a new entry for a new key. Copy each new key.
       Hash the key. Select its bucket. Search the bucket chain.
       Add a new entry to the chain and insertion list.
       put "beta" -> 2 on an empty map    -> DT_OK, "beta" is last in order
       put "beta" -> 22 on that map       -> DT_OK, same position, new value
       an allocation failure              -> DT_ERR_CAPACITY, map unchanged
       cases/normal/map_basics.case */
    
    // This is the starting accumulator. 
    unsigned long long h = 14695981039346656037ULL;
    
    // Each byte is used to calculate the hash value.
    for (const unsigned char *p = (const unsigned char *)key; *p != '\0'; p++) {
        h ^= (unsigned long long)*p;
        h *= 1099511628211ULL;
    }

    // Since bucket_count=5, the remainder stays between 0-4.
    size_t bucket =h%m->bucket_count;

    // Current is the entry stored in the bucket
    // Example. -1 means no entry, 0 or 1 means there are entries
    int current = m->buckets[bucket];

    // If it is not empty, it checks if the key is the same
    while (current != -1){
        if (strcmp(m->keys[current], key) == 0){
            m->values[current] = v;                 // Current coresponds to array position in values.
            return DT_OK;
        }
        current = m->next[current];                // Check the next entry in the same bucket
    }

    // If there's no available space, it creates a new one.
    // Example: A new allocated map has 0 for len and 0 for capacity.
    if (m->len == m->capacity){
        size_t new_capacity = (m->capacity==0) ? 4 : m->capacity*2;

        char **new_keys = malloc(new_capacity *sizeof(*new_keys));

        dt_value *new_values = malloc(new_capacity * sizeof(*new_values));

        int *new_next = malloc(new_capacity * sizeof(*new_next));

        char **new_order = malloc(new_capacity * sizeof(*new_order));

        // If allocation fails
        if (new_keys==NULL || new_values == NULL || new_next == NULL || new_order == NULL){
            free(new_keys);
            free(new_values);
            free(new_next);
            free(new_order);

            return DT_ERR_CAPACITY;
        }

        // This copies the existing entries into the larger storage.
        for(size_t i = 0; i<m->len; i++){
            new_keys[i] = m->keys[i];
            new_values[i] = m->values[i];
            new_next[i] = m->next[i];
            new_order[i] = m->order[i];
        }
        // Frees the old storage.
        free(m->keys);
        free(m->values);
        free(m->next);
        free(m->order);

        // Binds the new storage as the active storage.
        m->keys = new_keys;
        m->values = new_values;
        m->next = new_next;
        m->order = new_order;
        m->capacity = new_capacity;
    }

    // This part adds a new key as entry.
    // Map creates its own copy of the key.
    char *new_key = malloc(strlen(key) + 1);        // allocation

    if(new_key == NULL){
        return DT_ERR_CAPACITY;
    }

    strcpy(new_key, key);                           // copying the key

    size_t index = m->len;                          // gets the index of the new  entry

    m->keys[index] = new_key;                       // inserts the new_key to the keys
    m->values[index] = v;                           // inserts values
    m->next[index] = m->buckets[bucket];            // adds a new link to the next
    m->buckets[bucket] = (int)index;                // makes the new entry the first entry in the bucket

    m->order[index] = new_key;                      // stores the key in the insertion order
    m->len++;                                       // increases the number of entries

    return DT_OK;

    // (void)m;
    // (void)key;
    // (void)v;
    // return DT_ERR_CAPACITY;
}

/*
 * dt_map_get writes the value for key to *out.
 * It returns DT_ERR_KEY and does not change *out when the key is absent.
 * An absent key differs from a nil value.
 */
dt_status dt_map_get(const dt_map *m, const char *key, dt_value *out)
{
    /* TODO: Return DT_ERR_KEY when the key is absent.
       Preserve *out after this error. A nil value can be present.
       after put "beta" -> 22:
         dt_map_get(m, "beta", &out)   -> DT_OK, *out is the integer 22
         dt_map_get(m, "ghost", &out)  -> DT_ERR_KEY, *out untouched
       cases/normal/map_basics.case, cases/boundary/map_missing_key.case */
    
    // This is the starting accumulator.
    unsigned long long h = 14695981039346656037ULL;

    // Each byte is used to calculate the hash value.
    for (const unsigned char *p = (const unsigned char *)key; *p != '\0'; p++) {
        h ^= (unsigned long long)*p;
        h *= 1099511628211ULL;
    }

    // Since bucket_count=5, the remainder stays between 0-4.
    size_t bucket =h%m->bucket_count;

    // Current is the entry stored in the bucket
    // Example. -1 means no entry, 0 or 1 means there are entries
    int current = m->buckets[bucket];

    // If it is not empty, it checks if the key is the same
    while (current != -1){
        if (strcmp(m->keys[current], key) == 0){
            *out = m->values[current];
            return DT_OK;
        }
        current = m->next[current];                          // Check the next entry in the same bucket
    }

    return DT_ERR_KEY;
 
       // (void)m;
    // (void)key;
    // (void)out;
    // return DT_ERR_KEY;
}

/*
 * dt_map_remove removes key from its bucket and insertion position.
 * It releases the copied key. It returns DT_ERR_KEY when the key is absent.
 */
dt_status dt_map_remove(dt_map *m, const char *key)
{
    /* TODO: Remove the entry from its bucket and insertion position.
       Release the copied key. Return DT_ERR_KEY when the key is absent.
       a map holding alpha, beta, gamma:
         dt_map_remove(m, "alpha")  -> DT_OK, order is now beta, gamma
         dt_map_remove(m, "ghost")  -> DT_ERR_KEY, nothing changes
       reinserting "alpha" appends it after "gamma"
       cases/normal/map_basics.case, cases/boundary/map_remove_missing_key.case */
    // This is the starting accumulator.
    unsigned long long h = 14695981039346656037ULL;

    // Each byte is used to calculate the hash value.
    for (const unsigned char *p = (const unsigned char *)key; *p != '\0'; p++) {
        h ^= (unsigned long long)*p;
        h *= 1099511628211ULL;
    }

    // Since bucket_count=5, the remainder stays between 0-4.
    size_t bucket =h%m->bucket_count;

    // Current is the entry stored in the bucket.
    int current = m->buckets[bucket];
    int previous = -1;

    while (current != -1){
        if (strcmp(m->keys[current], key) == 0){
            break;
        }
        previous = current;
        current = m->next[current];
    }

    if (current == -1){
        return DT_ERR_KEY;
    }

    if (previous == -1){
        m->buckets[bucket] = m->next[current];
    } else {
        m->next[previous] = m->next[current];
    }

    for (size_t i=0; i<m->len; i++) {
        if (m->order[i] == m->keys[current]){
            for (size_t j=i; j+1 <m->len; j++){
                m->order[j] = m->order[j+1];
            }
            break;
        }
    }

    free(m->keys[current]);

    for (size_t i=(size_t)current; i+1<m->len; i++){
        m->keys[i] = m->keys[i+1];
        m->values[i] = m->values[i+1];
        m->next[i] = m->next[i+1];
    }

    m->len--;

    for (size_t i = 0; i<m->bucket_count; i++){
        m->buckets[i] = -1;
    }

    for (size_t i=0; i<m->len; i++){
        unsigned long long new_hash = 14695981039346656037ULL;
    
        for (const unsigned char *p = (const unsigned char *)m->keys[i]; *p != '\0'; p++) {
            new_hash ^= (unsigned long long)*p;
            new_hash *= 1099511628211ULL;
        }

        size_t new_bucket = new_hash%m->bucket_count;

        m->next[i] = m->buckets[new_bucket];
        m->buckets[new_bucket] = (int)i;
    }
    return DT_OK;
    

       // (void)m;
    // (void)key;
    // return DT_ERR_KEY;
}

/*
 * dt_map_key_at writes the key at insertion position index to *out.
 * It returns DT_ERR_RANGE and does not change *out for an invalid index.
 */
dt_status dt_map_key_at(const dt_map *m, size_t index, const char **out)
{
    /* TODO: Write the key at the specified insertion position to *out.
       Return DT_ERR_RANGE for an invalid position. Preserve *out after this error.
       The printer uses this order.
       a map holding alpha, beta, gamma:
         dt_map_key_at(m, 0, &out)  -> DT_OK, *out = "alpha"
         dt_map_key_at(m, 3, &out)  -> DT_ERR_RANGE, *out untouched
       cases/normal/map_basics.case */
    if (index >= m->len){
        return DT_ERR_RANGE;
    }
    *out = m->order[index];

    return DT_OK;
    // (void)m;
    // (void)index;
    // (void)out;
    // return DT_ERR_RANGE;
}
