#pragma once

#include "types.h"

/**
 * The flag for full barrier synchronization.
 */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

struct ArchThreadRegisters
{
  uint64  rip;       //   0
  uint64  cs;        //   8
  uint64  rflags;    //  16
  uint64  rax;       //  24
  uint64  rcx;       //  32
  uint64  rdx;       //  40
  uint64  rbx;       //  48
  uint64  rsp;       //  56
  uint64  rbp;       //  64
  uint64  rsi;       //  72
  uint64  rdi;       //  80
  uint64  r8;        //  88
  uint64  r9;        //  96
  uint64  r10;       // 104
  uint64  r11;       // 112
  uint64  r12;       // 120
  uint64  r13;       // 128
  uint64  r14;       // 136
  uint64  r15;       // 144
  uint64  ds;        // 152
  uint64  es;        // 160
  uint64  fs;        // 168
  uint64  gs;        // 176
  uint64  ss;        // 184
  uint64  rsp0;      // 192
  uint64  cr3;       // 200
  uint32  fpu[28];   // 208
};

class Thread;
class ArchMemory;

/**
 * this is where the thread info for task switching is stored
 *
 */
extern ArchThreadRegisters *currentThreadRegisters;
extern Thread *currentThread;

/**
 * Collection of architecture dependant code concerning Task Switching
 *
 */
class ArchThreads
{
public:

/**
 * allocates space for the currentThreadRegisters
 */
  static void initialise();

/**
 * creates the ArchThreadRegisters for a kernel thread
 * @param info where the ArchThreadRegisters is saved
 * @param start_function instruction pointer is set so start function
 * @param stack stackpointer
 */
  static void createKernelRegisters(ArchThreadRegisters *&info, void* start_function, void* kernel_stack);

/**
 * creates the ArchThreadRegisters for a user thread
 * @param info where the ArchThreadRegisters is saved
 * @param start_function instruction pointer is set so start function
 * @param user_stack pointer to the userstack
 * @param kernel_stack pointer to the kernel stack
 */
  static void createUserRegisters(ArchThreadRegisters *&info, void* start_function, void* user_stack, void* kernel_stack);

/**
 * changes an existing ArchThreadRegisters so that execution will start / continue
 * at the function specified
 * it does not change anything else, and if the thread info / thread was currently
 * executing something else this will lead to a lot of problems
 * USE WITH CARE, or better, don't use at all if you're a student
 * @param the ArchThreadRegisters that we are going to mangle
 * @param start_function instruction pointer for the next instruction that gets executed
 */
  static void changeInstructionPointer(ArchThreadRegisters *info, void* function);

/**
 *
 * on x86: invokes int65, whose handler facilitates a task switch
 *
 */
  static void yield();

/**
 * sets a threads page map level 4
 *
 * @param *thread Pointer to Thread Object
 * @param arch_memory the arch memory object for the address space
 */
  static void setAddressSpace(Thread *thread, ArchMemory& arch_memory);

  template <class T>
  static bool atomic_is_lock_free()
  {
    return
      sizeof (T) == 1 ||
      sizeof (T) == 2 ||
      sizeof (T) == 4 ||
      sizeof (T) == 8;
  }

  template <class T>
  static T atomic_load(T& target)
  {
    T ret;
    __asm__ __volatile__(
      "mfence\n\t"
      "mov%z0 %1, %0\n\t"
      : "=r"(ret) : "m"(&target)
    );
    return ret;
  }

  template <class T>
  static void atomic_store(T& target, T value)
  {
    __asm__ __volatile__(
      "mov%z0 %0, %1\n\t"
      "mfence\n\t"
      :: "r"(value), "m"(&target)
    );
  }

  template <class T>
  static T atomic_exchange(T& target, T val)
  {
    __asm__ __volatile__(
      "lock xchg%z0 %0, %1\n\t"
      : "+r"(val) : "m"(&target)
    );
    return val;
  }

  template <class T>
  static bool atomic_compare_exchange(T& target, T& expected, T desired)
  {
    bool ret;
    __asm__ __volatile__(
      "lock cmpxchg%z2 %2, %3\n\t"
      "setz %b0\n\t"
      : "=r"(ret), "+a"(expected), "+r"(desired) : "m"(&target)
    );
    return ret;
  }

  template <class T>
  static T atomic_fetch_add(T& target, T inc)
  {
    __asm__ __volatile__(
      "lock xadd%z0 %0, %1\n\t"
      : "+r"(inc) : "m"(&target)
    );
    return inc;
  }

  template <class T>
  static T atomic_fetch_and(T& target, T mask)
  {
    T t = atomic_load(target);
    while (!atomic_compare_exchange(target, t, t & mask));
    return t;
  }

  template <class T>
  static T atomic_fetch_or(T& target, T mask)
  {
    T t = atomic_load(target);
    while (!atomic_compare_exchange(target, t, t | mask));
    return t;
  }

  template <class T>
  static T atomic_fetch_xor(T& target, T mask)
  {
    T t = atomic_load(target);
    while (!atomic_compare_exchange(target, t, t ^ mask));
    return t;
  }

  template <class T>
  static T atomic_fetch_sub(T& target, T dec)
  {
    return atomic_fetch_add(target, -dec);
  }

  template <class T>
  static T atomic_add_fetch(T& target, T inc)
  {
    return atomic_fetch_add(target, inc) + inc;
  }

  template <class T>
  static T atomic_sub_fetch(T& target, T dec)
  {
    return atomic_fetch_sub(target, dec) - dec;
  }

  template <class T>
  static T atomic_and_fetch(T& target, T mask)
  {
    return atomic_fetch_and(target, mask) & mask;
  }

  template <class T>
  static T atomic_or_fetch(T& target, T mask)
  {
    return atomic_fetch_or(target, mask) | mask;
  }

  template <class T>
  static T atomic_xor_fetch(T& target, T mask)
  {
    return atomic_fetch_xor(target, mask) ^ mask;
  }

  static void atomic_fence()
  {
    asm("mfence");
  }

/**
 *
 * @param thread
 * @param userspace_register
 *
 */
  static void printThreadRegisters(Thread *thread, size_t userspace_registers, bool verbose = true);
  static void printThreadRegisters(Thread *thread, bool verbose = true);

  /**
   * check thread state for sanity
   * @param thread
   */
  static void debugCheckNewThread(Thread* thread);

private:
  /**
   * creates the ArchThreadRegisters for a thread (common setup for kernel and user registers)
   * @param info where the ArchThreadRegisters is saved
   * @param start_function instruction pointer is set to start function
   * @param stack stackpointer
   */
  static void createBaseThreadRegisters(ArchThreadRegisters *&info, void* start_function, void* stack);
};
