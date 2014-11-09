// thready.h
//
// https://github.com/tylerneylon/thready
//
// A small library that provides safe and easy cross-platform threads.
// This library focuses on small messages to be passed between threads to help
// ensure safety; this is sometimes referred to as the actor model:
// http://en.wikipedia.org/wiki/Actor_model
//

// TODO Update all comments.

// Design:
//
// A thready message is a json_Item.
//
// Ownership of json_Items is given to thready when they are sent
// and then to the receiver when received.
//
// Non-main threads are automatically started in an event loop, and operate
// by means of a given callback.
//
// The main thread may either be send-only, or, in order to receive messages,
// must regularly call thready__runloop(callback) which will provide any
// received messages.
//

#pragma once

#include "../json/json.h"


// Typedefs.

typedef void *thready__Id;  // An identifier for a thread.

// A function to receive messages.
typedef void  (*thready__Receiver)(json_Item msg, thready__Id from);


// The thready interface.

thready__Id thready__create (thready__Receiver receiver);
void        thready__exit   ();

thready__Id thready__runloop(thready__Receiver receiver, int blocking);
thready__Id thready__send   (json_Item msg, thready__Id to);
thready__Id thready__my_id  ();


// Constants

extern const thready__Id thready__error;
extern const thready__Id thready__success;

// Use these constants with thready__runloop for readable parameter values.
#define thready__nonblocking 0
#define thready__blocking    1
