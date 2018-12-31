#include <stdlib.h>
#include "m99cc.h"

DataType *new_data_type(int dt) {
  DataType *newdt = malloc(sizeof(DataType));
  newdt->dtype = dt;
  newdt->pointer_type = NULL;
  return newdt;
}

DataType *new_data_pointer(DataType *dt) {
  DataType *newdt = malloc(sizeof(DataType));
  newdt->dtype = DT_PNT;
  newdt->pointer_type = dt;
  return newdt;
}
