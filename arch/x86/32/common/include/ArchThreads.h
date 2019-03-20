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
  uint32  eip;       // 0
  uint32  cs;        // 4
  uint32  eflags;    // 8
  uint32  eax;       // 12
  uint32  ecx;       // 16
  uint32  edx;       // 20
  uint32  ebx;       // 24
  uint32  esp;       // 28
  uint32  ebp;       // 32
  uint32  esi;       // 36
  uint32  edi;       // 40
  uint32  ds;        // 44
  uint32  es;        // 48
  uint32  fs;        // 52
  uint32  gs;        // 56
  uint32  ss;        // 60
  uint32  esp0;      // 64
  uint32  cr3;       // 68
  uint32  fpu[27];   // 72
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
 * sets a threads CR3 register to the given page dir / etc. defining its address space
 *
 * @param *thread Pointer to Thread Object
 * @param arch_memory a reference to the arch memory object to use
 */
  static void setAddressSpace(Thread *thread, ArchMemory& arch_memory);

  template <class T>
  constexpr static bool atomic_is_implemented()
  {
    return atomic_is_lock_free<T>();
  }

  template <class T>
  constexpr static bool atomic_is_lock_free()
  {
    return
      sizeof (T) == 1 ||
      sizeof (T) == 2 ||
      sizeof (T) == 4;
  }

  template <class T>
  static T atomic_load(T& target)
  {
    T ret;
    atomic_fence();
    __asm__ __volatile__(
      "mov%z0 %1, %0\n\t"
      : "=r"(ret) : "m"(target)
    );
    return ret;
  }

  template <class T>
  static void atomic_store(T& target, T value)
  {
    __asm__ __volatile__(
      "mov%z0 %0, %1\n\t"
      :: "r"(value), "m"(target)
    );
    atomic_fence();
  }

  template <class T>
  static T atomic_exchange(T& target, T val)
  {
    __asm__ __volatile__(
      "lock xchg%z0 %0, %1\n\t"
      : "+r"(val) : "m"(target)
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
      : "=r"(ret), "+a"(expected), "+r"(desired) : "m"(target)
    );
    return ret;
  }

  template <class T>
  static T atomic_fetch_add(T& target, T inc)
  {
    __asm__ __volatile__(
      "lock xadd%z0 %0, %1\n\t"
      : "+r"(inc) : "m"(target)
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

  static size_t atomic_test_set_lock(size_t& lock, size_t new_value)
  {
    return atomic_exchange<size_t>(lock, new_value);
  }

/**
 *
 * @param thread
 * @param userspace_register
 *
 */
  static void printThreadRegisters(Thread *thread, uint32 userspace_registers, bool verbose = true);
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

