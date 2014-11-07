// json.h
//
// https://github.com/tylerneylon/cstructs-json
//
// A way to serialize / deserialize between cstructs
// objects and strings using JSON.
//

#pragma once

#include "../cstructs/cstructs.h"

typedef union {
  char * string;
  long   boolean;
  Array  array;
  Map    object;
  double number;
} json_ItemValue;

typedef enum {
  item_string,
  item_number,
  item_object,
  item_array,
  item_true,
  item_false,
  item_null,
  item_error
} json_ItemType;

typedef struct {
  json_ItemType type;
  json_ItemValue value;
} json_Item;

// Main functions to parse or jsonify.

// Returns the tail of json_str after the first valid json object.
// On error, *item has type item_error with a message in value.string.
char *json_parse(char *json_str, json_Item *item);

char *json_stringify(json_Item item);         // Smaller output with no extra whitespace.
char *json_pretty_stringify(json_Item item);  // Human-friendly output with more whitespace.

// Helper function to deallocate items.
// release_item is designed for Array; free_item is designed for Map.
// They accept a void * type to be a valid releaser for a Map/Array.

// This does NOT free the item itself; only its contents, recursively.
void json_release_item(void *item);

// Frees both the contents and the item itself; does strictly more than release_item.
void json_free_item(void *item);

// map__Hash and equality functions for use in a Map keyed by strings.
int json_str_hash(void *str_void_ptr);
int json_str_eq(void *str_void_ptr1, void *str_void_ptr2);

#include "jsonutil.h"

