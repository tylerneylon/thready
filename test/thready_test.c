// thready_test.c
//
// https://github.com/tylerneylon/thready
//
// For testing the awesome thready library.
//

#include "thready/thready.h"

#include "ctest.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include "winutil.h"
#endif

#pragma warning (disable : 4244)


////////////////////////////////////////////////////////////////////////////////
// Simple test

void bg_handle_msg(void *msg, thready__Id from) {
  test_printf("tid %p; got a message.\n", thready__my_id());
  test_str_eq(msg, "howdy");
  free(msg);

  void *reply = strdup("howdy back");
  thready__send(reply, from);
}

void fg_handle_msg(void *msg, thready__Id from) {
  test_printf("tid %p; got a message.\n", thready__my_id());
  test_str_eq(msg, "howdy back");
  free(msg);
}

int simple_test() {
  thready__Id other = thready__create(bg_handle_msg);
  test_printf("main test body: my tid=%p new tid=%p.\n", thready__my_id(), other);
  void *msg = strdup("howdy");
  thready__send(msg, other);
  thready__runloop(fg_handle_msg, thready__blocking);
  return test_success;
}


////////////////////////////////////////////////////////////////////////////////
// Exit test

void get_msg_and_exit(void *msg, thready__Id from) {
  thready__send(NULL, from);
  thready__exit();
  test_failed("We shouldn't get here since it's after thready__exit.\n");
}

void do_nothing_receiver(void *msg, thready__Id from) {}

int exit_test() {
  thready__Id other = thready__create(get_msg_and_exit);
  thready__send(NULL, other);

  // Block until the other thread sends us something in order to give that
  // thread a chance to run before this test concludes.
  thready__runloop(do_nothing_receiver, thready__blocking);
  
  return test_success;
}


////////////////////////////////////////////////////////////////////////////////
// Four thread test

void get_msg1(void *msg, thready__Id from) {
  int msg_int = (int)(intptr_t)msg;
  test_that(msg_int == 1);
  thready__send(NULL, from);
}

void get_msg2(void *msg, thready__Id from) {
  int msg_int = (int)(intptr_t)msg;
  test_that(msg_int == 2);
  thready__send(NULL, from);
}

void get_msg3(void *msg, thready__Id from) {
  int msg_int = (int)(intptr_t)msg;
  test_that(msg_int == 3);
  thready__send(NULL, from);
}

static int numcallbacks = 0;

void main_get_msg(void *msg, thready__Id from) {
  numcallbacks++;
}

int four_thread_test() {

  // The four threads in question are the main thread and
  // three created threads.

  thready__Id id1 = thready__create(get_msg1);
  thready__Id id2 = thready__create(get_msg2);
  thready__Id id3 = thready__create(get_msg3);

  thready__send((void *)(intptr_t)1, id1);
  thready__send((void *)(intptr_t)2, id2);
  thready__send((void *)(intptr_t)3, id3);

  // Wait for all other threads to receive their message so they
  // have a chance to run, and possibly fail if something is wrong.
  while (numcallbacks < 3) {
    thready__runloop(main_get_msg, thready__blocking);
  }

  return test_success;
}


////////////////////////////////////////////////////////////////////////////////
// Scale test

// This is the scale_test's main thread's id.
// This is set once by the main thread before any child threads are created,
// and treated as ready-only from there.
static thready__Id main_id;

void scale_child_get_msg(void *msg, thready__Id from) {
  if (msg) {
    // This is another thread's id.
    thready__Id to = (thready__Id)msg;
    thready__send(NULL, to);
  }

  thready__send(NULL, main_id);
}

// This computes the number of messages received by child threads.
// We know this because each child sends a single NULL msg to the
// main thread for every message it receives.
// Only the main thread sees or touches this value.
static int num_msg_recd = 0;

void scale_main_get_msg(void *msg, thready__Id from) {
  num_msg_recd++;
}

#define num_kids 100

int scale_test() {

  const int msg_per_kid = 100;

  // Create num_kids other threads, tell each about msg_per_kid others at
  // random, and each sends a message to those msg_per_kid others. We make sure
  // a total of exactly 2 * num_kids * msg_per_kid messages are received.
  //
  // This test fails by either freezing if an unexpectedly low number of
  // messages are sent, or by failing normally if too many messages are
  // sent and caught by the nonblocking runloop cycles at the test's end.
  
  test_printf("Beginning scale_test.\n");
  main_id = thready__my_id();
  srand(time(NULL));
  thready__Id ids[num_kids];

  test_printf("About to create %d threads.\n", num_kids);
  for (int i = 0; i < num_kids; ++i) {
    ids[i] = thready__create(scale_child_get_msg);
  }

  test_printf("About to send out %d ids to each child thread.\n", msg_per_kid);
  for (int i = 0; i < num_kids; ++i) {
    for (int j = 0; j < msg_per_kid; ++j) {
      thready__send(ids[rand() % num_kids], ids[i]);
    }
  }

  test_printf("Waiting for num_msg_recd to reach 2000.\n");
  int msg_goal = 2 * num_kids * msg_per_kid;
  while (num_msg_recd < msg_goal) {
    thready__runloop(scale_main_get_msg, thready__blocking);
  }
  // Check for any superfluous messages.
  for (int i = 0; i < 1000; ++i) {
    thready__runloop(scale_main_get_msg, thready__nonblocking);
  }
  test_that(num_msg_recd == msg_goal);

  return test_success;
}


////////////////////////////////////////////////////////////////////////////////
// Main

int main(int argc, char **argv) {
  set_verbose(0);  // Set this to 1 to help debug tests.

  start_all_tests(argv[0]);
  run_tests(
    simple_test, exit_test, four_thread_test, scale_test
  );
  return end_all_tests();
}
