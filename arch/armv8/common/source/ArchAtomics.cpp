#include <mm/new.h>
#include "SpinLock.h"
#include "ArchAtomics.h"
#include "ArchInterrupts.h"

#ifndef VIRTUALIZED_QEMU
  SpinLock ArchAtomics::global_atomic_lock("");
#endif

void ArchAtomics::initialise()
{
  #ifndef VIRTUALIZED_QEMU
    new (&ArchAtomics::global_atomic_lock) SpinLock("global_atomic_lock");
  #endif
}

void ArchAtomics::fence()
{
  asm("dmb sy");
}

size_t ArchAtomics::test_set_lock(size_t& lock, size_t new_value)
{
  #ifdef VIRTUALIZED_QEMU
    return exchange<size_t>(lock, new_value);
  #else
    bool int_state = ArchInterrupts::disableInterrupts();

    volatile size_t ret = lock;
    ((volatile size_t&)lock) = new_value;

    if (int_state)
    {
      ArchInterrupts::enableInterrupts();
    }

    return ret;
  #endif
}

#ifndef VIRTUALIZED_QEMU
  template <>
  void ArchAtomics::store<size_t>(size_t& target, size_t value)
  {
    // avoid locking the global atomic lock
    // since the Lock class wants to use atomic store
    // therefore use the interrupt-disabling test_set_lock
    test_set_lock(target, value);
  }
#endif
