// thready_test.c
//
// https://github.com/tylerneylon/thready
//
// For testing the awesome thready library.
//

#include "thready/thready.h"

#include "ctest.h"
#include <stdio.h>
#include <string.h>

// TODO Review if these are needed.

#define true 1
#define false 0

#define array_size(x) (sizeof(x) / sizeof(x[0]))

// Notes to myself:
//
// test_printf(fmt, ...)
// test_that(cond)
// test_str_eq(s1, s2)
// test_failed(fmt, ...)
//


////////////////////////////////////////////////////////////////////////////////
// Simple test

void bg_handle_msg(json_Item msg, thready__Id from) {
  test_printf("tid %p; got a message.\n", thready__my_id());
  test_that(msg.type == item_string);
  test_str_eq(item_str(msg), "howdy");
  json_release_item(&msg);

  json_Item reply = copy_str_item("howdy back");
  thready__send(reply, from);
}

void fg_handle_msg(json_Item msg, thready__Id from) {
  test_printf("tid %p; got a message.\n", thready__my_id());
  test_that(msg.type == item_string);
  test_str_eq(item_str(msg), "howdy back");
  json_release_item(&msg);
}

int simple_test() {
  thready__Id other = thready__create(bg_handle_msg);
  test_printf("main test body: my tid=%p new tid=%p.\n", thready__my_id(), other);
  json_Item msg = copy_str_item("howdy");
  thready__send(msg, other);
  thready__runloop(fg_handle_msg, thready__blocking);
  return test_success;
}


////////////////////////////////////////////////////////////////////////////////
// Exit test

// TODO


////////////////////////////////////////////////////////////////////////////////
// Main

int main(int argc, char **argv) {
  set_verbose(0);  // Set this to 1 to help debug tests.

  start_all_tests(argv[0]);
  run_tests(
    simple_test
  );
  return end_all_tests();
}
