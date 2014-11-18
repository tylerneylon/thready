# thready

*A simple message-passing thread library in C.*

This is a small, efficient, cross-platform threading library that
handles concurrency control so you don't have to.
It runs on windows, mac, and ubuntu.

## Memory-sharing philosophy

Thready is primarily built around the actor model, meaning that each thread is thought of as an
independent actor with minimal information flow between them. Traditionally, messages sent
between actors are passed by value so that there is no question of ownership about them.
However, `thready` modifies this by passing a single `void *` by value and encouraging the usage
pattern that ownership of memory is passed from the sender of a message to the receiver.

Conceptually, communication between threads in thready is handled thusly:

    void thread1_fn() {
      thready__Id thread2 = thready__create(thread2_callback);
      void *my_message = allocate_and_populate_message();
      thready__send(my_message, thread2);
    }

    void thread2_callback(void *message, thready__Id from) {
      work_with_message(message);
      deallocate(message);
    }

The interesting thing about this code is that there are zero concurrency controls involved in
this transaction - things like mutexes, condition variables, or read-write locks. These
elements are handled within `thready`.

## API

The thready API consists of five functions that you may call, and one callback that you
may implement.

---
### `thready__create(thready__Receiver receiver)`

This function creates a new thread and begins that thread in an efficient run loop that will
dispatch all incoming messages to the given receiver. The `receiver` parameter is a function
pointer with parameters and types as described under `thready__callback` below.

Threads created this way are efficient in that they do not
busy-wait and they'll be awoken as soon as any incoming messages are available. These threads do
not hold any internal concurrency locks while your code is processing a message, so that your
code cannot implicitly block other threads.

This function returns a `thready__Id` value that can be passed in to `thready__send` in order
to send messages to the new thread.

The new thread can be terminated by having it call `thready__exit`.

This thread may return the value `thready__error` if there is an error, such as that the
system-determined thread limit has been reached.

---
### `thready__exit()`

This function terminates the thread it is called from.

---
### `thready__runloop(receiver, int blocking)`

This function gives the current thread a chance to receive messages sent to it by
`thready__send`. Threads created with `thready__create` should *not* call this function, as
they are automatically put into a thready run loop. Threads not created with `thready__create` -
such as the main thread of your process - *should* call this function regularly if they intend to
receive `thready` messages.

The `receiver` parameter is a function pointer to a function with the parameters and types
described under `thready__callback` below.

The `blocking` parameter can be given either the value `thready__blocking` or
`thready__nonblocking`. A nonblocking call returns as soon as all messages pending at the start of
the runloop have been dispatched. A blocking call waits until at least one message has arrived and
been dispatched before returning. Blocking is handled efficiently in that the cpu is never kept busy
while the inbox of a thread is empty.

---
### `thready__callback(void *msg, thready__Id from)`

This is a function you implement that receives messages.
When you send a `receiver` function pointer to either `thready__create` or `thready__runloop`, it
must have these parameter types and a `void` return value.

It is up to you to determine
how to pass information using `msg`. One approach is to define a C struct which is allocated by
the sender and then deallocated by the receiver. Another, described below, is to integrate with
`cstructs-json` to work with json-style data.

---
### `thready__send(void *msg, thready__Id to)`

This sends the given `msg` to the given `thread` recipient.

This returns a `thready__Id` value which may be either `thready__error` or `thready__success`.
One example of an error condition is that the given `to` id is unknown to `thready`.

---
### `thready__my_id()`

This returns the `thready__Id` of the calling thread.  Thready ids are different from either windows
thread ids or posix thread ids. Like other concepts of a thread id, this id is unique and consistent
within the process for the lifetime of the thread.

## Working with json messages

You are free to use the `msg` pointer in whatever way you choose - `thready` treats it as an opaque
object passed around by value.
One method of using the `msg` pointer is to send in json-format data
by way of the
[`cstructs-json`](https://github.com/tylerneylon/cstructs-json) library.

Here is a suggested usage pattern for working with json data:

    // In the sender:
    json_Item item = get_my_data();
    thready__send(item_copy_ptr(item), to);  // The receiver owns the item.

    // In the receiver, we get `void *msg`:
    json_Item item = *(json_Item *)msg;
    work_with_item(item);
    json_free_item(msg);

It is possible, in C-to-C use of json messages, to override the `string` value type
for use as a general pointer. In this way, arbitrary C data structures can be passed around in
an augmented json format. The `cstructs-json` library simply frees a string pointer when the
item is released. You may either use this to help manage your memory - by letting the standard
  json release free your struct for you - or you may set your pointer to NULL, in which case
  the standard json release will do nothing, meaning that you will free the data yourself.
