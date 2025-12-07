// q-gcc: `pkg-config glib-2.0 --cflags --libs` --
#include <glib.h>
#include <stdio.h>

// Function to print a key-value pair
void print_pair(gpointer key, gpointer value, gpointer user_data) {
    const char *str_key = (const char *)key;
    int int_value = GPOINTER_TO_INT(value);
    printf("  %s: %d\n", str_key, int_value);
}

int main() {
    // Create a new hash table
    // First parameter: hash function for strings (g_str_hash)
    // Second parameter: comparison function for strings (g_str_equal)
    GHashTable *hash = g_hash_table_new(g_str_hash, g_str_equal);

    printf("Created a new hash table.\n\n");

    // Insert some key-value pairs
    g_hash_table_insert(hash, (gpointer)"apple", GINT_TO_POINTER(5));
    g_hash_table_insert(hash, (gpointer)"banana", GINT_TO_POINTER(3));
    g_hash_table_insert(hash, (gpointer)"orange", GINT_TO_POINTER(7));


    g_hash_table_foreach(hash, print_pair, NULL);

    return 0;
}
