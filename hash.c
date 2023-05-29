/*
 * This file contains the implementation of a simple chained hash table.  For
 * simplicity, the hash table is set up to store void* values.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "hash.h"

/*
 * The initial capacity of the hash table array.
 */
#define INITIAL_CAPACITY 128

/*
 * Threshold on load factor over which we double the capacity of the table.
 */
#define LOAD_FACTOR_THR 5

/*
 * This structure is used to represent key/value pairs in the hash table.  We
 * also store a link to the next link in a chain directly in the association,
 * making it double as a linked list node.
 */
struct association {
  char* key;
  void* value;
  struct association* next;
};


/*
 * This structure is used to represent the hash table itself.
 */
struct hash {
  struct association** table;
  unsigned int capacity;
  unsigned int num_elems;
};

/*
 * Helper function to initialize a hash table's table array to a given capacity.
 */
void _hash_table_init(struct hash* hash, unsigned int capacity) {
  hash->table = malloc(capacity * sizeof(struct association*));
  assert(hash->table);
  memset(hash->table, 0, capacity * sizeof(struct association*));
  hash->capacity = capacity;
  hash->num_elems = 0;
}


/*
 * Create a new hash table.
 */
struct hash* hash_create() {
  struct hash* hash = malloc(sizeof(struct hash));
  assert(hash);
  _hash_table_init(hash, INITIAL_CAPACITY);
  return hash;
}


/*
 * This function frees all memory allocated to an association structure.
 */
void _association_free(struct association* assoc) {
  free(assoc->key);
  free(assoc);
}


/*
 * Free the memory associated with a hash table.
 */
void hash_free(struct hash* hash) {
  assert(hash);

  /*
   * Free all of the association structs stored in the table.
   */
  for (int i = 0; i < hash->capacity; i++) {
    struct association* next, * cur = hash->table[i];
    while (cur != NULL) {
      next = cur->next;
      _association_free(cur);
      cur = next;
    }
  }

  free(hash->table);
  free(hash);
}


float _hash_load_factor(struct hash* hash) {
  return hash->num_elems / (float)hash->capacity;
}


/*
 * The DJB hash function: http://www.cse.yorku.ca/~oz/hash.html.
 */
unsigned int _djb_hash(const char* key) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++)) {
    hash = ((hash << 5) + hash) + c;  // hash * 33 + c
  }
  return hash;
}


/*
 * Helper function to double the capacity of a hash table if it gets too full.
 */
void _hash_resize(struct hash* hash) {

  /*
   * Remember the old table array and its capacity and re-initialize the
   * hash with a new table array with twice the capacity.
   */
  struct association** old_table = hash->table;
  unsigned int old_capacity = hash->capacity;
  _hash_table_init(hash, old_capacity * 2);

  /*
   * Loop through the old table and re-hash all the old elements via
   * hash_insert().  This will handle updating the table's size for us.  By
   * definition, this will not cause a recursive call to _hash_resize(),
   * since the new table's capacity is doubled.  Free all the old assocation
   * structures.
   */
  for (int i = 0; i < old_capacity; i++) {
    struct association* cur = old_table[i];
    while (cur != NULL) {
      struct association* tmp = cur;
      hash_insert(hash, cur->key, cur->value);
      cur = cur->next;
      _association_free(tmp);
    }
  }

  free(old_table);

}


/*
 * This function allocates an association structure and initializes it with
 * a given key and value.  Memory is allocated and a copy of the key is made.
 */
struct association* _association_create(const char* key, void* value) {
  struct association* assoc = malloc(sizeof(struct association));
  int l = strlen(key);
  assoc->key = malloc((l + 1) * sizeof(char));
  strncpy(assoc->key, key, l + 1);
  assoc->value = value;
  return assoc;
}


/*
 * Inserts (or updates) a value with a given key into a hash table.
 */
void hash_insert(struct hash* hash, const char* key, void* value) {
  assert(hash);
  assert(key);

  /*
   * Double capacity of hash table if needed.
   */
  if (_hash_load_factor(hash) > LOAD_FACTOR_THR) {
    _hash_resize(hash);
  }

  /*
   * Compute a hash value for the given key and mod to convert it to an index.
   */
  unsigned int hashval = _djb_hash(key);
  unsigned int idx = hashval % hash->capacity;

  /*
   * Find the key if it already exists in the table.
   */
  struct association* cur = hash->table[idx];
  struct association* prev = NULL;
  while (cur != NULL) {
    if (!strcmp(key, cur->key)) {
      break;
    }
    prev = cur;
    cur = cur->next;
  }

  if (cur != NULL) {
    /*
     * If the key we're looking for exists in the table, assign the new value.
     */
    cur->value = value;
  } else {
    /*
     * If the user wants to add a new key/value pair into the table, allocate
     * a new association structure for it and put the new association at the
     * head of the chain for its bucket.
     */
    cur = _association_create(key, value);
    if (hash->table[idx] != NULL) {
      cur->next = hash->table[idx];
    } else {
      cur->next = NULL;
    }
    hash->table[idx] = cur;
    hash->num_elems++;
  }
}


/*
 * Removes a value with a given key from a hash table, if it exists there.
 */
void hash_remove(struct hash* hash, const char* key) {
  assert(hash);
  assert(key);

  /*
   * Compute a hash value for the given key and mod to convert it to an index.
   */
  unsigned int hashval = _djb_hash(key);
  unsigned int idx = hashval % hash->capacity;

  /*
   * Find the key if it already exists in the table.
   */
  struct association* cur = hash->table[idx];
  struct association* prev = NULL;
  while (cur != NULL) {
    if (!strcmp(key, cur->key)) {
      break;
    }
    prev = cur;
    cur = cur->next;
  }

  /*
   * If the key we're looking for exists in the table, then remove the
   * association from the table by updating link pointers.
   */
  if (cur != NULL) {
    if (prev != NULL) {
      prev->next = cur->next;
    } else {
      hash->table[idx] = cur->next;
    }

    _association_free(cur);
    hash->num_elems--;
  }
}


/*
 * Returns the value of an element with a given key from a hash table.
 * Returns NULL if the key doesn't exist in the hash table.
 */
void* hash_get(struct hash* hash, const char* key) {
  assert(hash);
  assert(key);

  /*
   * Compute a hash value for the given key and mod to convert it to an index.
   */
  unsigned int hashval = _djb_hash(key);
  unsigned int idx = hashval % hash->capacity;

  /*
   * Find the key if it exists in the table.
   */
  struct association* cur = hash->table[idx];
  while (cur != NULL) {
    if (!strcmp(key, cur->key)) {
      return cur->value;
    }
    cur = cur->next;
  }

  /*
   * If we made it here, we haven't found the key we're looking for.
   */
  return NULL;
}


/*
 * Returns 1 if the hash table contains the given key or 0 otherwise.
 */
int hash_contains(struct hash* hash, const char* key) {
  assert(hash);
  assert(key);

  /*
   * Compute a hash value for the given key and mod to convert it to an index.
   */
  unsigned int hashval = _djb_hash(key);
  unsigned int idx = hashval % hash->capacity;

  /*
   * Find the key if it exists in the table.
   */
  struct association* cur = hash->table[idx];
  while (cur != NULL) {
    if (!strcmp(key, cur->key)) {
      return 1;
    }
    cur = cur->next;
  }

  /*
   * If we made it here, we haven't found the key we're looking for.
   */
  return 0;
}


/*****************************************************************************
 **
 ** Iterator definitions
 **
 *****************************************************************************/

/*
 * This is the structure representing a hash table iterator.
 */
struct hash_iter {
  struct hash* hash;
  struct association* next;
  int next_idx;
};


/*
 * Utility function to update the `next` pointer for the hash table iterator to
 * point to the next element in the hash table.
 */
void _update_hash_iter_next(struct hash_iter* iter) {
  if (iter->next != NULL) {
    iter->next = iter->next->next;
  }
  if (iter->next == NULL) {
    int i = iter->next_idx + 1;
    while (i < iter->hash->capacity && iter->hash->table[i] == NULL) {
      i++;
    }
    if (i < iter->hash->capacity) {
      iter->next_idx = i;
      iter->next = iter->hash->table[i];
    }
  }
}


/*
 * Create a new iterator over a hash table.
 */
struct hash_iter* hash_iter_create(struct hash* hash) {
  assert(hash);
  struct hash_iter* iter = malloc(sizeof(struct hash_iter));
  iter->hash = hash;
  iter->next = NULL;
  iter->next_idx = -1;
  _update_hash_iter_next(iter);
  return iter;
}

/*
 * Free a hash table iterator.
 */
void hash_iter_free(struct hash_iter* iter) {
  assert(iter);
  free(iter);
}

/*
 * Function to determine whether there are more elements in a hash iterator.
 * Returns 1 if there are more elements or 0 if there are none.
 */
int hash_iter_has_next(struct hash_iter* iter) {
  assert(iter);
  return iter->next != NULL;
}

/*
 * Returns the next value in the hash and advances the iterator.  If `key_ptr`
 * is not NULL, it is set to point to the key associated with the returned
 * value.  The pointer stored in `key_ptr` should not be freed by the
 * caller!
 */
void* hash_iter_next(struct hash_iter* iter, char** key_ptr) {
  assert(iter);
  assert(iter->next);
  struct association* curr = iter->next;
  _update_hash_iter_next(iter);
  if (key_ptr != NULL) {
    *key_ptr = curr->key;
  }
  return curr->value;
}
