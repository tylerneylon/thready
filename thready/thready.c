// thready.c

#include "thready.h"

#include <pthread.h>


// Internal functions.

static void *thread_runner(void *recevier) {
  // TODO
  return NULL;
}

// Public functions.

thready__Id thready__create(thready__ReceiverFn receiver) {
  pthread_t thread;
  int err = pthread_create(&thread,        // receive thread id
                           NULL,           // NULL --> use default attributes
                           thread_runner,  // init function
                           NULL);          // init function arg
  if (err) return thready__error;

  // We only get here on success.
  return (thready__Id)thread;
}

void thready__send(json_Item msg, thready__Id to) {
  // TODO
}

thready__Id thready__my_id() {
  // TODO
  return 0;
}
