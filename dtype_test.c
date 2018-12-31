#include <stdlib.h>
#include "m99cc.h"

int runtest_data_type() {
  DataType *d;
  DataType *p;
  d = new_data_type(DT_CHAR);
  p = new_data_pointer(d);
  if (d->dtype != DT_CHAR || d->pointer_type != NULL ||
      d->dtype != DT_PNT || d->pointer_type->dtype != DT_CHAR) {
    return -1;
  }
  free(d);
  free(p);

  d = new_data_type(DT_INT);
  p = new_data_pointer(d);
  if (d->dtype != DT_INT || d->pointer_type != NULL ||
      d->dtype != DT_PNT || d->pointer_type->dtype != DT_INT) {
    return -1;
  }
  free(d);
  free(p);

  d = new_data_type(DT_VOID);
  p = new_data_pointer(d);
  if (d->dtype != DT_VOID || d->pointer_type != NULL ||
      d->dtype != DT_PNT || d->pointer_type->dtype != DT_VOID) {
    return -1;
  }
  free(d);
  free(p);
  return 0;
}
