// thready.c

#include "thready.h"

#include "../cstructs/cstructs.h"

#include <pthread.h>


// Internal types and data.

typedef struct {
  json_Item item;
  thready__Id to;
} envelope;

// Each item in msg_queue is an envelope.
static Array msg_queue = NULL;

// This is the mutex for msg_queue.
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


// Internal functions.

static void init_if_needed() {

  // Run this function only once.
  static int is_initialized = 0;
  if (is_initialized) return;
  is_initialized = 1;

  // This array is never deleted.
  msg_queue = array__new(32, sizeof(envelope));
}

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

void thready__runloop(thready__ReceiverFn receiver) {
  // TODO
}


void thready__send(json_Item msg, thready__Id to) {
  // Protect access to the msg_queue.
  pthread_mutex_lock(&mutex);
  array__new_val(msg_queue, envelope) = (envelope){ .item = msg, .to = to };
  pthread_mutex_unlock(&mutex);
}

thready__Id thready__my_id() {
  return (thready__Id)pthread_self();
}
