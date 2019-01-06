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

void print_data_type(DataType *data_type) {
  fprintf(stderr, "%d", data_type->dtype);
  if (data_type->dtype == DT_PNT) {
    fprintf(stderr, "->");
    print_data_type(data_type->pointer_type);
  }
}

void dump_symbols(Map *symbol_table) {
  int symbol_number = symbol_table->keys->len;
  fprintf(stderr, "Total %d symbols present.\n", symbol_number);
  for (int cnt = 0; cnt < symbol_number; cnt++) {
    Symbol *sym = (Symbol *)symbol_table->vals->data[cnt];
    char *name = (char *)symbol_table->keys->data[cnt];
    fprintf(stderr, "name: %s, type: %d, address: %d, ", name,
            sym->type, (int)sym->address);
    fprintf(stderr, "data_type: ");
    print_data_type(sym->data_type);
    fprintf(stderr, "\n");
  }
}

void dump_struct_table(Map *struct_table) {
  int struct_number = struct_table->keys->len;
  fprintf(stderr, "Total %d structs present.\n", struct_number);
  for (int cnt = 0; cnt < struct_number; cnt++) {
    fprintf(stderr, "Struct: %s\n", (char *)struct_table->keys->data[cnt]);
    Vector *struct_members = struct_table->vals->data[cnt];
    for (int m_cnt = 0; m_cnt < struct_members->len; m_cnt++) {
      StructMember *sm = struct_members->data[m_cnt];
      fprintf(stderr, "member name: %s, data_type: ", sm->name);
      print_data_type(sm->data_type);
      fprintf(stderr, ", Address: %d\n", sm->address);
    }
  }
}
