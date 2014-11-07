// jsonutil.c
//
// https://github.com/tylerneylon/cstructs-json
//

#include "jsonutil.h"

#include <stdio.h>

#define true 1
#define false 0

#define array_size(x) (sizeof(x) / sizeof(x[0]))

// Internal; checks the start of fmt against item and leaves fmt pointing
// to the unparsed tail. Returns true iff item matches the start of fmt.
static int json_item_has_format_(json_Item item, char **fmt) {
  if (**fmt == '[') {
    if (item.type != item_array) return false;
    (*fmt)++;
    for (int i = 0;; ++i) {
      if (**fmt == '\0') {
        fprintf(stderr, "Error: json_item_has_format: format ended mid-array.\n");
        return false;
      }
      if (**fmt == ']') {
        (*fmt)++;
        return true;
      }
      if (i >= item.value.array->count) return false;
      if (!json_item_has_format_(item_at(item, i), fmt)) return false;
      if (**fmt != ']' && **fmt != ',') {
        char *err_fmt = "Error: json_item_has_format expected , or ] at tail: '%s'.\n";
        fprintf(stderr, err_fmt, *fmt);
        return false;
      }
      if (**fmt == ',') (*fmt)++;
    }
  }

  char          type_chars[] = {'\'',        't',       'f',        'n',       '#'};
  json_ItemType item_types[] = {item_string, item_true, item_false, item_null, item_number};

  for (int i = 0; i < array_size(type_chars); ++i) {
    if (**fmt == type_chars[i]) {
      (*fmt)++;
      return item.type == item_types[i];
    }
  }

  char *err_fmt = "Error: json_item_has_format: parse error at tail: '%s'.\n";
  fprintf(stderr, err_fmt, *fmt);
  return false;
}

int json_item_has_format(json_Item item, char *format) {
  if (format == NULL) {
    fprintf(stderr, "Error: json_item_has_format given format=NULL.\n");
    return false;
  }
  return json_item_has_format_(item, &format);
}

json_Item json_item_or_error(map__key_value *pair) {
  if (pair == NULL) return error_item;
  return *(json_Item *)pair->value;
}
