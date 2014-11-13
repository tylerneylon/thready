// pthreads_win.h
//
// https://github.com/tylerneylon/thready
//
// A pthreads-similar wrapper around windows threading functions.
//

#include <windows.h>


///////////////////////////////////////////////////////////////////////////////
// Thread control.

typedef DWORD pthread_t;

int pthread_create(pthread_t *thread, void *attr, void *(*start_fn)(void *), void *arg);
void pthread_exit(void *exit_value);
#define pthread_self GetCurrentThreadId


///////////////////////////////////////////////////////////////////////////////
// Mutex.

#define pthread_mutex_t CRITICAL_SECTION
#define PTHREAD_MUTEX_INITIALIZER {(void*)-1,-1,0,0,0,0}

// All values `m` here have type `pthread_mutex_t *`.
#define pthread_mutex_lock(m) EnterCriticalSection(m)
#define pthread_mutex_unlock(m) LeaveCriticalSection(m)
#define pthread_mutex_release(m) DeleteCriticalSection(m)

// Note: pthread_mutex_release is not actually a pthreads function, but seems
// to be required for the windows implementation.


///////////////////////////////////////////////////////////////////////////////
// Condition variable.

#define pthread_cond_t CONDITION_VARIABLE
#define PTHREAD_COND_INITIALIZER {0}

void pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
void pthread_cond_signal(pthread_cond_t *cond);


///////////////////////////////////////////////////////////////////////////////
// Read-write lock.

#define pthread_rwlock_t SRWLOCK
#define PTHREAD_RWLOCK_INITIALIZER SRWLOCK_INIT

#define pthread_rwlock_wrlock(lock) AcquireSRWLockExclusive(lock)
#define pthread_rwlock_rdlock(lock) AcquireSRWLockShared(lock)
#define pthread_rwlock_wrunlock(lock) ReleaseSRWLockExclusive(lock)
#define pthread_rwlock_rdunlock(lock) ReleaseSRWLockShared(lock)

// The two unlock functions are not actually pthreads functions. They are
// modifications to simplify the windows wrapper implementation. 


///////////////////////////////////////////////////////////////////////////////
// One-time initialization.

#define pthread_once_t INIT_ONCE
#define PTHREAD_ONCE_INIT INIT_ONCE_STATIC_INIT

void pthread_once(pthread_once_t *once_control, void(*init)());


