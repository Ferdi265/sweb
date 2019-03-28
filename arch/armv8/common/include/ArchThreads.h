#pragma once

#include "types.h"

/**
 * The flag for full barrier synchronization.
 */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

typedef struct
{
	size_t q1;
	size_t q2;
}NeonQ;

struct ArchThreadRegisters
{
	size_t X[31];
	NeonQ  Q[32];
	size_t SPSR;
	size_t ELR;
	size_t SP;
	size_t TTBR0;
	size_t SP_SM;
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
 *
 */
  static void initialise();

/**
 * creates the ArchThreadRegisters for a kernel thread
 * @param info where the ArchThreadRegisters is saved
 * @param start_function instruction pointer is set so start function
 * @param stack stackpointer
 */
  static void createKernelRegisters(ArchThreadRegisters *&info, void* start_function, void* stack);

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
 * creates the ArchThreadRegisters for a user thread
 * @param info where the ArchThreadRegisters is saved
 * @param start_function instruction pointer is set so start function
 * @param user_stack pointer to the userstack
 * @param kernel_stack pointer to the kernel stack
 */
  static void createUserRegisters(ArchThreadRegisters *&info, void* start_function, void* user_stack, void* kernel_stack);

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
};

