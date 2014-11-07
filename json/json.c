// json.c
//
// https://github.com/tylerneylon/cstructs-json
//

#include "json.h"

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define array__free_but_leave_elements free

// This compiles as nothing when DEBUG is not defined.
#include "debug_hooks.h"

#define true  1
#define false 0

// Define clo:char -> #leading ones; 0 if char == 0.
#ifdef _WIN32
#include "winutil.h"

int clo(char c) {
  if (c == 0) { return 0; }
  unsigned long num_ones;
  _BitScanReverse(&num_ones, ~(c << 24));
  return (int)(31 - num_ones);
}
#else
#define clo(c) (c ? __builtin_clz(~(c << 24)) : 0)
#endif


// Globals.

static char *encoded_chars = "bfnrt\"\\";
static char *decoded_chars = "\b\f\n\r\t\"\\";


// UTF-8 functions.
//
// These are from:
// https://gist.github.com/tylerneylon/9773800

static int decode_code_point(char **s) {
  int k = clo(**s);               // Count # of leading 1 bits (0 if **s == 0).
  int mask = (1 << (8 - k)) - 1;  // All 1's with k leading 0's.
  int value = **s & mask;
  for (++(*s), --k; k > 0 && **s; --k, ++(*s)) {  // Note that k = #total bytes, or 0.
    value <<= 6;
    value += (**s & 0x3F);
  }
  return value;
}

// Assumes that code is <= 0x10FFFF.
// Ensures that nothing will be written at or beyond end.
static void encode_code_point(char **s, char *end, int code) {
  char val[4];
  int lead_byte_max = 0x7F;
  int val_index = 0;
  while (code > lead_byte_max) {
    val[val_index++] = (code & 0x3F) | 0x80;
    code >>= 6;
    lead_byte_max >>= (val_index == 1 ? 2 : 1);
  }
  val[val_index++] = (code & lead_byte_max) | (~lead_byte_max << 1);
  while (val_index-- && *s < end) {
    **s = val[val_index];
    (*s)++;
  }
}

// Returns 0 if no split was needed.
static int split_into_surrogates(int code, int *surr1, int *surr2) {
  if (code <= 0xFFFF) return 0;
  *surr2 = 0xDC00 | (code & 0x3FF);        // Save the low 10 bits.
  code >>= 10;                             // Drop the low 10 bits.
  *surr1 = 0xD800 | (code & 0x03F);        // Save the next 6 bits.
  *surr1 |= ((code & 0x7C0) - 0x40) << 6;  // Insert the last 5 bits less 1.
  return 1;
}

// Expects to be used in a loop and see all code points in *code. Start *old at 0;
// this function updates *old for you - don't change it. Returns 0 when *code is
// the 1st of a surrogate pair; otherwise use *code as the final code point.
static int join_from_surrogates(int *old, int *code) {
  if (*old) *code = (((*old & 0x3FF) + 0x40) << 10) + (*code & 0x3FF);
  *old = ((*code & 0xD800) == 0xD800 ? *code : 0);
  return !(*old);
}


// Internal functions.

// Using macros is a hacky-but-not-insane (in my opinion)
// way to ensure these 'functions' are inlined.

#define next_token(input) \
  input++; \
  input += strspn(input, " \t\r\n");

#define rngmap(base, low, hi, too_low, too_hi) \
  (c < low ? too_low : (c <= hi ? c - low + base : too_hi))

// The value of this expression is -1 for invalid hex digits, or
// 0-15 for valid digits; it's based on the char in variable c.
#define value_of_hex_char \
  rngmap(10, 'A', 'F', rngmap(0, '0', '9', -1, -1), rngmap(10, 'a', 'f', -1, -1))

// Both parse_hex_code_pt and parse_string_unit follow these rules:
// At start: *input is the first hex char to read.
// At end: c is the last-read char, *input is the first char not yet read.

#define parse_hex_code_pt(char_array, input, val) \
  for (int i = 0; c && i < 4; ++i) { \
    c = *input++; \
    int vohc = value_of_hex_char; \
    if (vohc < 0) break; \
    val <<= 4; \
    val += vohc; \
  }

#define parse_string_unit(char_array, input) \
  c = *input++; \
  if (c != '\\') { \
    array__new_val(char_array, char) = c; \
  } else { \
    c = *input++; \
    if (c == 'u') { \
      int val = 0; \
      parse_hex_code_pt(char_array, input, val); \
      if (join_from_surrogates(&old_val, &val)) { \
        char *s, buf[4]; \
        s = buf; \
        encode_code_point(&s, s + 4, val); \
        ArrayStruct buf_holder = { .count = (int)(s - buf), .item_size = 1, .items = buf }; \
        array__append_array(char_array, &buf_holder); \
      } \
    } else { \
      char *esc_ptr = strchr(encoded_chars, c); \
      if (esc_ptr) c = decoded_chars[esc_ptr - encoded_chars]; \
      array__new_val(char_array, char) = c; \
    } \
  }

// A consolidated function for error cleanup in parse_{value,frac_part,exponent}.
static char *err(json_Item *item, json_Item *subitem, char *msg, long index, Array arr, Map obj) {
  if (subitem) *item = *subitem;
  if (msg) {
    item->type = item_error;
    asprintf(&item->value.string, "Error: %s at index %ld", msg, index);
  }
  if (subitem) subitem->type = item_null;
  if (arr) array__delete(arr);
  if (obj) map__delete(obj);
  return NULL;
}

static char *parse_exponent(json_Item *item, char *input, char *start) {
  if (*input == 'e' || *input == 'E') {
    input++;
    if (*input == '\0') return err(item, 0, "expected exponent", input - start, 0, 0);
    double exp = 0.0;
    double sign = (*input == '-' ? -1.0 : 1.0);
    if (*input == '-' || *input == '+') input++;
    if (!isdigit(*input)) return err(item, 0, "expected digit", input - start, 0, 0);
    do {
      exp *= 10.0;
      exp += (*input - '0');
      input++;
    } while (isdigit(*input));
    item->value.number = item->value.number * pow(10.0, exp * sign);
  }
  return input - 1;  // Leave the number pointing at its last character.
}

static char *parse_frac_part(json_Item *item, char *input, char *start) {
  if (*input == '.') {
    input++;
    double w = 0.1;
    if (!isdigit(*input)) return err(item, 0, "expected digit after .", input - start, 0, 0);
    do {
      item->value.number += w * (*input - '0');
      w *= 0.1;
      input++;
    } while (isdigit(*input));
  }
  return parse_exponent(item, input, start);
}

// Assumes there's no leading whitespace.
// At the end, the input points to the last
// character of the parsed value.
static char *parse_value(json_Item *item, char *input, char *start) {

  // Parse a number.
  int sign = 1;
  if (*input == '-') {
    sign = -1;
    input++;
  }

  if (isdigit(*input)) {
    item->type = item_number;
    item->value.number = 0.0;

    if (*input != '0') {
      do {
        item->value.number *= 10.0;
        item->value.number += (*input - '0');
        input++;
      } while (isdigit(*input));
    } else {
      input++;
    }
    input = parse_frac_part(item, input, start);
    if (input) item->value.number *= sign;
    return input;

  } else if (sign == -1) {
    // This is a - without a number after it.
    return err(item, 0, "expected digit", input - start, 0, 0);
  }

  // Parse a string.
  if (*input == '"') {
    input++;
    item->type = item_string;
    Array char_array = array__new(16, sizeof(char));
    char c = 1;
    int old_val = 0;
    while (c && *input != '"') {
      parse_string_unit(char_array, input);
    }
    // Check for he end of the string before we see a closing quote.
    if (c == '\0') return err(item, 0, "string not closed", input - start, char_array, 0);
    array__new_val(char_array, char) = '\0';  // Terminating null.

    item->value.string = char_array->items;
    array__free_but_leave_elements(char_array);

    return input;
  }

  // Parse an array.
  if (*input == '[') {
    next_token(input);

    Array array = array__new(8, sizeof(json_Item));
    array->releaser = json_release_item;
    item->type = item_array;
    item->value.array = array;

    while (*input != ']') {
      if (array->count) {
        if (*input != ',') return err(item, 0, "expected ']' or ','", input - start, array, 0);
        next_token(input);
      }
      json_Item *subitem = (json_Item *)array__new_ptr(array);
      input = parse_value(subitem, input, start);
      if (input == NULL) return err(item, subitem, 0, 0, array, 0);
      next_token(input);
    }

    return input;
  }

  // Parse an object.
  if (*input == '{') {
    next_token(input);
    Map obj = map__new(json_str_hash, json_str_eq);
    obj->key_releaser = free;
    obj->value_releaser = json_free_item;
    item->type = item_object;
    item->value.object = obj;
    while (*input != '}') {
      if (obj->count) {
        if (*input != ',') return err(item, 0, "expected '}' or ','", input - start, 0, obj);
        next_token(input);
      }

      // Parse the key, which should be a string.
      if (*input != '"') { return err(item, 0, "expected '\"'", input - start, 0, obj); }
      json_Item key;
      input = parse_value(&key, input, start);
      if (input == NULL) {
        *item = key;
        map__delete(obj);
        return NULL;
      }

      // Set up placeholder objects in the map.
      json_Item *subitem = (json_Item *)malloc(sizeof(json_Item));
      map__set(obj, key.value.string, subitem);  // obj takes ownership of both pointers passed in.

      // Parse the separating colon.
      next_token(input);
      if (*input != ':') return err(item, subitem, "expected ':'", input - start, 0, obj);

      // Parse the value of this key.
      next_token(input);
      input = parse_value(subitem, input, start);
      if (input == NULL) return err(item, subitem, 0, 0, 0, obj);
      next_token(input);
    }
    return input;
  }

  // Parse a literal: true, false, or null.
  char *literals[3] = {"false", "true", "null"};
  size_t lit_len[3] = {5, 4, 4};
  json_ItemType types[3] = {item_false, item_true, item_null};

  for (int i = 0; i < 3; ++i) {
    if (*input != literals[i][0]) continue;
    if (strncmp(input, literals[i], lit_len[i]) != 0) {
      char msg[32];
      snprintf(msg, 32, "expected '%s'", literals[i]);
      return err(item, 0, msg, input - start, 0, 0);
    }
    item->type = types[i];
    item->value.boolean = (i < 2) ? i : 0;  // The 0 literal is for the null case.
    return input + (lit_len[i] - 1);
  }

  // If we get here, the string is not well-formed.
  return err(item, 0, "unexpected character", input - start, 0, 0);
}

// Expects the input array to have items of type char *.
static int array_printf(Array array, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int return_value = vasprintf((char **)array__new_ptr(array), fmt, args);
  va_end(args);
  return return_value;
}

// Expects the array to have items of type char *. Caller owns the newly-allocated return val.
static char *array_join(Array array) {
  int total_len = 0;
  array__for(char **, str_ptr, array, idx) total_len += strlen(*str_ptr);
  char *str = malloc(total_len + 1);  // +1 for the final null.
  char *tail = str;
  array__for(char **, str_ptr, array, idx) tail = stpcpy(tail, *str_ptr);
  return str;
}

#define new_elt(arr) array__new_val(arr, char)

#define add_u_escaped_pt(arr, pt) \
  new_elt(arr) = '\\'; \
  new_elt(arr) = 'u'; \
  for (int i = 0; i < 4; ++i, pt >>= 4) hex_digits[3 - i] = hex[pt % 16]; \
  for (int i = 0; i < 4; ++i) new_elt(arr) = hex_digits[i];

// Caller must free the returned string.
static char *escaped_str(char *s) {
  Array array = array__new(4, sizeof(char));

  // These are used in add_u_escaped_pt to encode non-ascii unicode characters.
  static char *hex = "0123456789ABCDEF";
  char hex_digits[4];

  while (*s) {
    int code_pt = decode_code_point(&s);
    char *esc_ptr = strchr(decoded_chars, code_pt);
    if (esc_ptr) {
      new_elt(array) = '\\';
      new_elt(array) = encoded_chars[esc_ptr - decoded_chars];
    } else if (code_pt < 0x80) {
      new_elt(array) = code_pt;
    } else {
      int surr1, surr2;
      if (split_into_surrogates(code_pt, &surr1, &surr2)) {
        add_u_escaped_pt(array, surr1);
        add_u_escaped_pt(array, surr2);
      } else {
        add_u_escaped_pt(array, code_pt);
      }
    }
  }
  new_elt(array) = '\0';
  char *escaped_s = array->items;
  array__free_but_leave_elements(array);
  return escaped_s;
}

static void print_item(Array array, json_Item item, char *indent, int be_terse) {
  char *outer_indent = indent;
  if (!be_terse) { asprintf(&indent, "%s  ", indent); }  // Nest indents, except when terse.
  char *sep = be_terse ? "," : ",\n";
  char *spc = be_terse ? "" : " ";
  char *lit[] = { [item_true] = "true", [item_false] = "false", [item_null] = "null"};
  char *esc_s;  // Used to hold escaped strings.
  int i = 0;    // Used to index obj/arr items in loops within the switch.
  switch (item.type) {
    case item_string:
    case item_error:
      esc_s = escaped_str(item.value.string);
      array_printf(array, "\"%s\"", esc_s);
      free(esc_s);
      break;
    case item_true:
    case item_false:
    case item_null:
      array_printf(array, "%s", lit[item.type]);
      break;
    case item_number:
      array_printf(array, "%g", item.value.number);
      break;
    case item_array:
      array_printf(array, item.value.array->count && !be_terse ? "[\n" : "[");
      array__for(json_Item *, subitem, item.value.array, idx) {
        array_printf(array, "%s%s", (i++ ? sep : ""), indent);
        print_item(array, *subitem, indent, be_terse);
      }
      if (item.value.array->count && !be_terse) array_printf(array, "\n%s", outer_indent);
      array_printf(array, "]");
      break;
    case item_object:
      array_printf(array, item.value.object->count && !be_terse ? "{\n" : "{");
      map__for(pair, item.value.object) {
        array_printf(array, "%s%s\"%s\":%s", (i++ ? sep : ""), indent, (char *)pair->key, spc);
        print_item(array, *(json_Item *)pair->value, indent, be_terse);
      }
      if (item.value.object->count && !be_terse) array_printf(array, "\n%s", outer_indent);
      array_printf(array, "}");
      break;
  }
  if (!be_terse) free(indent);
}

static void free_at(void *ptr) {
  free(*(void **)ptr);
}

char *json_stringify_internal(json_Item item, int be_terse) {
  Array str_array = array__new(8, sizeof(char *));
  str_array->releaser = free_at;
  print_item(str_array, item, "" /* indent */, be_terse);
  char *json_str = array_join(str_array);
  array__delete(str_array);
  return json_str;
}


// Public functions.

char *json_parse(char *json_str, json_Item *item) {
  char *input = json_str + strspn(json_str, " \t\r\n" );  // Skip leading whitespace.
  input = parse_value(item, input, json_str);
  if (input) {
    next_token(input);  // Skip last parsed char and trailing whitespace.
  }
  return input;
}

char *json_stringify(json_Item item) {
  return json_stringify_internal(item, true /* be_terse */);
}

char *json_pretty_stringify(json_Item item) {
  return json_stringify_internal(item, false /* be_terse */);
}

void json_release_item(void *item_ptr) {
  json_Item *item = (json_Item *)item_ptr;
  if (item->type == item_string || item->type == item_error) free(item->value.string);
  if (item->type == item_object) map__delete(item->value.object);
  if (item->type == item_array) array__delete(item->value.array);
}

void json_free_item(void *item) {
  json_release_item(item);
  free(item);
}

int json_str_hash(void *str_void_ptr) {
  char *str = (char *)str_void_ptr;
  int h = *str;
  while (*str) {
    h *= 84207;
    h += *str++;
  }
  return h;
}

int json_str_eq(void *str_void_ptr1, void *str_void_ptr2) {
  return !strcmp(str_void_ptr1, str_void_ptr2);
}
