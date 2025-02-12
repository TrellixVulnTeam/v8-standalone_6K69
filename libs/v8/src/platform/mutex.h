// Copyright 2013 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef V8_PLATFORM_MUTEX_H_
#define V8_PLATFORM_MUTEX_H_

#include "../lazy-instance.h"
#if V8_OS_WIN
#include "../win32-headers.h"
#endif

#if V8_OS_POSIX
#include <pthread.h>  // NOLINT
#elif V8_OS_SA
#include <v8sa/mutex.h>
#endif

namespace v8 {
namespace internal {

// ----------------------------------------------------------------------------
// Mutex
//
// This class is a synchronization primitive that can be used to protect shared
// data from being simultaneously accessed by multiple threads. A mutex offers
// exclusive, non-recursive ownership semantics:
// - A calling thread owns a mutex from the time that it successfully calls
//   either |Lock()| or |TryLock()| until it calls |Unlock()|.
// - When a thread owns a mutex, all other threads will block (for calls to
//   |Lock()|) or receive a |false| return value (for |TryLock()|) if they
//   attempt to claim ownership of the mutex.
// A calling thread must not own the mutex prior to calling |Lock()| or
// |TryLock()|. The behavior of a program is undefined if a mutex is destroyed
// while still owned by some thread. The Mutex class is non-copyable.

class Mutex V8_FINAL {
 public:
  Mutex();
  ~Mutex();

  // Locks the given mutex. If the mutex is currently unlocked, it becomes
  // locked and owned by the calling thread, and immediately. If the mutex
  // is already locked by another thread, suspends the calling thread until
  // the mutex is unlocked.
  void Lock();

  // Unlocks the given mutex. The mutex is assumed to be locked and owned by
  // the calling thread on entrance.
  void Unlock();

  // Tries to lock the given mutex. Returns whether the mutex was
  // successfully locked.
  bool TryLock() V8_WARN_UNUSED_RESULT;

  // The implementation-defined native handle type.
#if V8_OS_POSIX
  typedef pthread_mutex_t NativeHandle;
#elif V8_OS_WIN
  typedef CRITICAL_SECTION NativeHandle;
#elif V8_OS_SA
  typedef v8sa::Mutex NativeHandle;
#endif

  NativeHandle& native_handle() {
    return native_handle_;
  }
  const NativeHandle& native_handle() const {
    return native_handle_;
  }

 private:
  NativeHandle native_handle_;
#ifdef DEBUG
  int level_;
#endif

  V8_INLINE void AssertHeldAndUnmark() {
#ifdef DEBUG
    ASSERT_EQ(1, level_);
    level_--;
#endif
  }

  V8_INLINE void AssertUnheldAndMark() {
#ifdef DEBUG
    ASSERT_EQ(0, level_);
    level_++;
#endif
  }

  friend class ConditionVariable;

  DISALLOW_COPY_AND_ASSIGN(Mutex);
};


// POD Mutex initialized lazily (i.e. the first time Pointer() is called).
// Usage:
//   static LazyMutex my_mutex = LAZY_MUTEX_INITIALIZER;
//
//   void my_function() {
//     LockGuard<Mutex> guard(my_mutex.Pointer());
//     // Do something.
//   }
//
typedef LazyStaticInstance<Mutex,
                           DefaultConstructTrait<Mutex>,
                           ThreadSafeInitOnceTrait>::type LazyMutex;

#define LAZY_MUTEX_INITIALIZER LAZY_STATIC_INSTANCE_INITIALIZER


// -----------------------------------------------------------------------------
// RecursiveMutex
//
// This class is a synchronization primitive that can be used to protect shared
// data from being simultaneously accessed by multiple threads. A recursive
// mutex offers exclusive, recursive ownership semantics:
// - A calling thread owns a recursive mutex for a period of time that starts
//   when it successfully calls either |Lock()| or |TryLock()|. During this
//   period, the thread may make additional calls to |Lock()| or |TryLock()|.
//   The period of ownership ends when the thread makes a matching number of
//   calls to |Unlock()|.
// - When a thread owns a recursive mutex, all other threads will block (for
//   calls to |Lock()|) or receive a |false| return value (for |TryLock()|) if
//   they attempt to claim ownership of the recursive mutex.
// - The maximum number of times that a recursive mutex may be locked is
//   unspecified, but after that number is reached, calls to |Lock()| will
//   probably abort the process and calls to |TryLock()| return false.
// The behavior of a program is undefined if a recursive mutex is destroyed
// while still owned by some thread. The RecursiveMutex class is non-copyable.

class RecursiveMutex V8_FINAL {
 public:
  RecursiveMutex();
  ~RecursiveMutex();

  // Locks the mutex. If another thread has already locked the mutex, a call to
  // |Lock()| will block execution until the lock is acquired. A thread may call
  // |Lock()| on a recursive mutex repeatedly. Ownership will only be released
  // after the thread makes a matching number of calls to |Unlock()|.
  // The behavior is undefined if the mutex is not unlocked before being
  // destroyed, i.e. some thread still owns it.
  void Lock();

  // Unlocks the mutex if its level of ownership is 1 (there was exactly one
  // more call to |Lock()| than there were calls to unlock() made by this
  // thread), reduces the level of ownership by 1 otherwise. The mutex must be
  // locked by the current thread of execution, otherwise, the behavior is
  // undefined.
  void Unlock();

  // Tries to lock the given mutex. Returns whether the mutex was
  // successfully locked.
  bool TryLock() V8_WARN_UNUSED_RESULT;

  // The implementation-defined native handle type.
  typedef Mutex::NativeHandle NativeHandle;

  NativeHandle& native_handle() {
    return native_handle_;
  }
  const NativeHandle& native_handle() const {
    return native_handle_;
  }

 private:
  NativeHandle native_handle_;
#ifdef DEBUG
  int level_;
#endif

  DISALLOW_COPY_AND_ASSIGN(RecursiveMutex);
};


// POD RecursiveMutex initialized lazily (i.e. the first time Pointer() is
// called).
// Usage:
//   static LazyRecursiveMutex my_mutex = LAZY_RECURSIVE_MUTEX_INITIALIZER;
//
//   void my_function() {
//     LockGuard<RecursiveMutex> guard(my_mutex.Pointer());
//     // Do something.
//   }
//
typedef LazyStaticInstance<RecursiveMutex,
                           DefaultConstructTrait<RecursiveMutex>,
                           ThreadSafeInitOnceTrait>::type LazyRecursiveMutex;

#define LAZY_RECURSIVE_MUTEX_INITIALIZER LAZY_STATIC_INSTANCE_INITIALIZER


// -----------------------------------------------------------------------------
// LockGuard
//
// This class is a mutex wrapper that provides a convenient RAII-style mechanism
// for owning a mutex for the duration of a scoped block.
// When a LockGuard object is created, it attempts to take ownership of the
// mutex it is given. When control leaves the scope in which the LockGuard
// object was created, the LockGuard is destructed and the mutex is released.
// The LockGuard class is non-copyable.

template <typename Mutex>
class LockGuard V8_FINAL {
 public:
  explicit LockGuard(Mutex* mutex) : mutex_(mutex) { mutex_->Lock(); }
  ~LockGuard() { mutex_->Unlock(); }

 private:
  Mutex* mutex_;

  DISALLOW_COPY_AND_ASSIGN(LockGuard);
};

} }  // namespace v8::internal

#endif  // V8_PLATFORM_MUTEX_H_
