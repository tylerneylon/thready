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

int simple_test() {
  return test_success;
}

int main(int argc, char **argv) {
  set_verbose(0);  // Set this to 1 to help debug tests.

  start_all_tests(argv[0]);
  run_tests(
    simple_test
  );
  return end_all_tests();
}
