// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_EXECUTION_H_
#define V8_EXECUTION_H_

#include "handles.h"

namespace v8 {
namespace internal {

// Flag used to set the interrupt causes.
enum InterruptFlag {
  INTERRUPT = 1 << 0,
  DEBUGBREAK = 1 << 1,
  DEBUGCOMMAND = 1 << 2,
  PREEMPT = 1 << 3,
  TERMINATE = 1 << 4,
  GC_REQUEST = 1 << 5,
  FULL_DEOPT = 1 << 6,
  INSTALL_CODE = 1 << 7,
  API_INTERRUPT = 1 << 8,
  DEOPT_MARKED_ALLOCATION_SITES = 1 << 9
};


class Execution V8_FINAL : public AllStatic {
 public:
  // Call a function, the caller supplies a receiver and an array
  // of arguments. Arguments are Object* type. After function returns,
  // pointers in 'args' might be invalid.
  //
  // *pending_exception tells whether the invoke resulted in
  // a pending exception.
  //
  // When convert_receiver is set, and the receiver is not an object,
  // and the function called is not in strict mode, receiver is converted to
  // an object.
  //
  MUST_USE_RESULT static MaybeHandle<Object> Call(
      Isolate* isolate,
      Handle<Object> callable,
      Handle<Object> receiver,
      int argc,
      Handle<Object> argv[],
      bool convert_receiver = false);

  // Construct object from function, the caller supplies an array of
  // arguments. Arguments are Object* type. After function returns,
  // pointers in 'args' might be invalid.
  //
  // *pending_exception tells whether the invoke resulted in
  // a pending exception.
  //
  MUST_USE_RESULT static MaybeHandle<Object> New(Handle<JSFunction> func,
                                                 int argc,
                                                 Handle<Object> argv[]);

  // Call a function, just like Call(), but make sure to silently catch
  // any thrown exceptions. The return value is either the result of
  // calling the function (if caught exception is false) or the exception
  // that occurred (if caught exception is true).
  static MaybeHandle<Object> TryCall(
      Handle<JSFunction> func,
      Handle<Object> receiver,
      int argc,
      Handle<Object> argv[],
      Handle<Object>* exception_out = NULL);

  // ECMA-262 9.3
  MUST_USE_RESULT static MaybeHandle<Object> ToNumber(
      Isolate* isolate, Handle<Object> obj);

  // ECMA-262 9.4
  MUST_USE_RESULT static MaybeHandle<Object> ToInteger(
      Isolate* isolate, Handle<Object> obj);

  // ECMA-262 9.5
  MUST_USE_RESULT static MaybeHandle<Object> ToInt32(
      Isolate* isolate, Handle<Object> obj);

  // ECMA-262 9.6
  MUST_USE_RESULT static MaybeHandle<Object> ToUint32(
      Isolate* isolate, Handle<Object> obj);

  // ECMA-262 9.8
  MUST_USE_RESULT static MaybeHandle<Object> ToString(
      Isolate* isolate, Handle<Object> obj);

  // ECMA-262 9.8
  MUST_USE_RESULT static MaybeHandle<Object> ToDetailString(
      Isolate* isolate, Handle<Object> obj);

  // ECMA-262 9.9
  MUST_USE_RESULT static MaybeHandle<Object> ToObject(
      Isolate* isolate, Handle<Object> obj);

  // Create a new date object from 'time'.
  MUST_USE_RESULT static MaybeHandle<Object> NewDate(
      Isolate* isolate, double time);

  // Create a new regular expression object from 'pattern' and 'flags'.
  MUST_USE_RESULT static MaybeHandle<JSRegExp> NewJSRegExp(
      Handle<String> pattern, Handle<String> flags);

  // Used to implement [] notation on strings (calls JS code)
  static Handle<Object> CharAt(Handle<String> str, uint32_t index);

  static Handle<Object> GetFunctionFor();
  MUST_USE_RESULT static MaybeHandle<JSFunction> InstantiateFunction(
      Handle<FunctionTemplateInfo> data);
  MUST_USE_RESULT static MaybeHandle<JSObject> InstantiateObject(
      Handle<ObjectTemplateInfo> data);
  MUST_USE_RESULT static MaybeHandle<Object> ConfigureInstance(
      Isolate* isolate,  Handle<Object> instance, Handle<Object> data);
  static Handle<String> GetStackTraceLine(Handle<Object> recv,
                                          Handle<JSFunction> fun,
                                          Handle<Object> pos,
                                          Handle<Object> is_global);
#ifdef ENABLE_DEBUGGER_SUPPORT
  static Object* DebugBreakHelper(Isolate* isolate);
  static void ProcessDebugMessages(Isolate* isolate, bool debug_command_only);
#endif

  // If the stack guard is triggered, but it is not an actual
  // stack overflow, then handle the interruption accordingly.
  MUST_USE_RESULT static MaybeObject* HandleStackGuardInterrupt(
      Isolate* isolate);

  // Get a function delegate (or undefined) for the given non-function
  // object. Used for support calling objects as functions.
  static Handle<Object> GetFunctionDelegate(Isolate* isolate,
                                            Handle<Object> object);
  MUST_USE_RESULT static MaybeHandle<Object> TryGetFunctionDelegate(
      Isolate* isolate,
      Handle<Object> object);

  // Get a function delegate (or undefined) for the given non-function
  // object. Used for support calling objects as constructors.
  static Handle<Object> GetConstructorDelegate(Isolate* isolate,
                                               Handle<Object> object);
  static MaybeHandle<Object> TryGetConstructorDelegate(Isolate* isolate,
                                                       Handle<Object> object);

  static void RunMicrotasks(Isolate* isolate);
  static void EnqueueMicrotask(Isolate* isolate, Handle<Object> microtask);
};


class ExecutionAccess;


// StackGuard contains the handling of the limits that are used to limit the
// number of nested invocations of JavaScript and the stack size used in each
// invocation.
class StackGuard V8_FINAL {
 public:
  // Pass the address beyond which the stack should not grow.  The stack
  // is assumed to grow downwards.
  void SetStackLimit(uintptr_t limit);

  // Threading support.
  char* ArchiveStackGuard(char* to);
  char* RestoreStackGuard(char* from);
  static int ArchiveSpacePerThread() { return sizeof(ThreadLocal); }
  void FreeThreadResources();
  // Sets up the default stack guard for this thread if it has not
  // already been set up.
  void InitThread(const ExecutionAccess& lock);
  // Clears the stack guard for this thread so it does not look as if
  // it has been set up.
  void ClearThread(const ExecutionAccess& lock);

  bool IsStackOverflow();
  bool IsPreempted();
  void Preempt();
  bool IsInterrupted();
  void Interrupt();
  bool IsTerminateExecution();
  void TerminateExecution();
  void CancelTerminateExecution();
#ifdef ENABLE_DEBUGGER_SUPPORT
  bool IsDebugBreak();
  void DebugBreak();
  bool IsDebugCommand();
  void DebugCommand();
#endif
  bool IsGCRequest();
  void RequestGC();
  bool IsInstallCodeRequest();
  void RequestInstallCode();
  bool IsFullDeopt();
  void FullDeopt();
  bool IsDeoptMarkedAllocationSites();
  void DeoptMarkedAllocationSites();
  void Continue(InterruptFlag after_what);

  void RequestInterrupt(InterruptCallback callback, void* data);
  void ClearInterrupt();
  bool IsAPIInterrupt();
  void InvokeInterruptCallback();

  // This provides an asynchronous read of the stack limits for the current
  // thread.  There are no locks protecting this, but it is assumed that you
  // have the global V8 lock if you are using multiple V8 threads.
  uintptr_t climit() {
    return thread_local_.climit_;
  }
  uintptr_t real_climit() {
    return thread_local_.real_climit_;
  }
  uintptr_t jslimit() {
    return thread_local_.jslimit_;
  }
  uintptr_t real_jslimit() {
    return thread_local_.real_jslimit_;
  }
  Address address_of_jslimit() {
    return reinterpret_cast<Address>(&thread_local_.jslimit_);
  }
  Address address_of_real_jslimit() {
    return reinterpret_cast<Address>(&thread_local_.real_jslimit_);
  }
  bool ShouldPostponeInterrupts();

 private:
  StackGuard();

  // You should hold the ExecutionAccess lock when calling this method.
  bool has_pending_interrupts(const ExecutionAccess& lock) {
    // Sanity check: We shouldn't be asking about pending interrupts
    // unless we're not postponing them anymore.
    ASSERT(!should_postpone_interrupts(lock));
    return thread_local_.interrupt_flags_ != 0;
  }

  // You should hold the ExecutionAccess lock when calling this method.
  bool should_postpone_interrupts(const ExecutionAccess& lock) {
    return thread_local_.postpone_interrupts_nesting_ > 0;
  }

  // You should hold the ExecutionAccess lock when calling this method.
  inline void set_interrupt_limits(const ExecutionAccess& lock);

  // Reset limits to actual values. For example after handling interrupt.
  // You should hold the ExecutionAccess lock when calling this method.
  inline void reset_limits(const ExecutionAccess& lock);

  // Enable or disable interrupts.
  void EnableInterrupts();
  void DisableInterrupts();

#if V8_TARGET_ARCH_X64 || V8_TARGET_ARCH_ARM64
  static const uintptr_t kInterruptLimit = V8_UINT64_C(0xfffffffffffffffe);
  static const uintptr_t kIllegalLimit = V8_UINT64_C(0xfffffffffffffff8);
#else
  static const uintptr_t kInterruptLimit = 0xfffffffe;
  static const uintptr_t kIllegalLimit = 0xfffffff8;
#endif

  class ThreadLocal V8_FINAL {
   public:
    ThreadLocal() { Clear(); }
    // You should hold the ExecutionAccess lock when you call Initialize or
    // Clear.
    void Clear();

    // Returns true if the heap's stack limits should be set, false if not.
    bool Initialize(Isolate* isolate);

    // The stack limit is split into a JavaScript and a C++ stack limit. These
    // two are the same except when running on a simulator where the C++ and
    // JavaScript stacks are separate. Each of the two stack limits have two
    // values. The one eith the real_ prefix is the actual stack limit
    // set for the VM. The one without the real_ prefix has the same value as
    // the actual stack limit except when there is an interruption (e.g. debug
    // break or preemption) in which case it is lowered to make stack checks
    // fail. Both the generated code and the runtime system check against the
    // one without the real_ prefix.
    uintptr_t real_jslimit_;  // Actual JavaScript stack limit set for the VM.
    uintptr_t jslimit_;
    uintptr_t real_climit_;  // Actual C++ stack limit set for the VM.
    uintptr_t climit_;

    int nesting_;
    int postpone_interrupts_nesting_;
    int interrupt_flags_;

    InterruptCallback interrupt_callback_;
    void* interrupt_callback_data_;
  };

  // TODO(isolates): Technically this could be calculated directly from a
  //                 pointer to StackGuard.
  Isolate* isolate_;
  ThreadLocal thread_local_;

  friend class Isolate;
  friend class StackLimitCheck;
  friend class PostponeInterruptsScope;

  DISALLOW_COPY_AND_ASSIGN(StackGuard);
};

} }  // namespace v8::internal

#endif  // V8_EXECUTION_H_
