/*
 * This file contains the declarations for a simple chained hash table.  For
 * simplicity, the hash table is set up to store void* values.  See hash.c for
 * implementation details.
 */

#ifndef __HASH_H
#define __HASH_H

/*
 * Structure used to represent a hash table.
 */
struct hash;

/*
 * Structure representing an iterator over a hash table.
 */
struct hash_iter;

/*
 * Create a new hash table.
 */
struct hash* hash_create();

/*
 * Free the memory associated with a hash table.
 */
void hash_free(struct hash* hash);

/*
 * Inserts (or updates) a value with a given key into a hash table.
 */
void hash_insert(struct hash* hash, const char* key, void* value);

/*
 * Removes a value with a given key from a hash table, if it exists there.
 */
void hash_remove(struct hash* hash, const char* key);

/*
 * Returns the value of an element with a given key from a hash table.
 * Returns NULL if the key doesn't exist in the hash table.
 */
void* hash_get(struct hash* hash, const char* key);

/*
 * Returns 1 if the hash table contains the given key or 0 otherwise.
 */
int hash_contains(struct hash* hash, const char* key);

/*
 * Create a new iterator over a hash table.
 */
struct hash_iter* hash_iter_create(struct hash* hash);

/*
 * Free a hash table iterator.
 */
void hash_iter_free(struct hash_iter* iter);

/*
 * Function to determine whether there are more elements in a hash iterator.
 * Returns 1 if there are more elements or 0 if there are none.
 */
int hash_iter_has_next(struct hash_iter* iter);

/*
 * Returns the next value in the hash and advances the iterator.  If `key_ptr`
 * is not NULL, it is set to point to the key associated with the returned
 * value.  The pointer stored in `key_ptr` should not be freed by the
 * caller!
 */
void* hash_iter_next(struct hash_iter* iter, char** key_ptr);

#endif
