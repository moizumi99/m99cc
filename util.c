#include "m99cc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Vector *new_vector() {
  Vector *vec = malloc(sizeof(Vector));
  vec->data = malloc(sizeof(void *) * 16);
  vec->capacity = 16;
  vec->len = 0;
  return vec;
}

void vec_push(Vector *vec, void *elem) {
  if (vec->capacity == vec->len) {
    vec->capacity *= 2;
    vec->data = realloc(vec->data, sizeof(void *) * vec->capacity);
  }
  vec->data[vec->len++] = elem;
}

void *vec_pop(Vector *vec) { return vec->data[vec->len--]; }

Map *new_map() {
  Map *map = malloc(sizeof(Map));
  map->keys = new_vector();
  map->vals = new_vector();
  return map;
}

void map_put(Map *map, char *key, void *val) {
  vec_push(map->keys, key);
  vec_push(map->vals, val);
}

void *map_get(Map *map, char *key) {
  for (int i = map->keys->len - 1; i >= 0; i--) {
    if (strcmp(map->keys->data[i], key) == 0) {
      return map->vals->data[i];
    }
  }
  return NULL;
}

// fof debugging

void dump_symbols(Map *symbol_table) {
  int symbol_number = symbol_table->keys->len;
  fprintf(stderr, "Total %d symbols present.\n", symbol_number);
  for (int cnt = 0; cnt < symbol_number; cnt++) {
    Symbol *sym = (Symbol *)symbol_table->vals->data[cnt];
    char *name = (char *)symbol_table->keys->data[cnt];
    fprintf(stderr, "name: %s, type: %d, data_type: %d, address: %d\n", name,
            sym->type, sym->data_type->dtype, (int)sym->address);
  }
}
