#include "ArchAtomics.h"

void ArchAtomics::fence()
{
  asm("mfence" ::: "memory");
}

size_t ArchAtomics::test_set_lock(size_t& lock, size_t new_value)
{
  return exchange<size_t>(lock, new_value);
}
