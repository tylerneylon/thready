// debug_hooks.h
//
// https://github.com/tylerneylon/cstructs-json
//
// This file is designed to be #include'd from cjson.c to
// enable memory allocation checks when the DEBUG macro
// is defined.
//
// It's not designed to be compiled directly.
//

#ifdef DEBUG

int cjson_net_obj_allocs = 0;
int cjson_net_arr_allocs = 0;

// Set up the array hooks.

Array array__new_dbg(int x, size_t y) {
  cjson_net_arr_allocs++;
  return array__new(x, y);
}
#define array__new array__new_dbg

void array__delete_dbg(Array x) {
  cjson_net_arr_allocs--;
  array__delete(x);
}
#define array__delete array__delete_dbg

#ifdef array__free_but_leave_elements
#undef array__free_but_leave_elements
#endif

void array__free_but_leave_elements(Array array) {
  cjson_net_arr_allocs--;
  free(array);
}

// Set up the map (object) hooks.

Map map__new_dbg(map__Hash x, map__Eq y) {
  cjson_net_obj_allocs++;
  return map__new(x, y);
}
#define map__new map__new_dbg

void map__delete_dbg(Map x) {
  cjson_net_obj_allocs--;
  map__delete(x);
}
#define map__delete map__delete_dbg

#endif
