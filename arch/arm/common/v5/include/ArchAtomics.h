#pragma once

#include "types.h"
#include "ArchInterrupts.h"

class ArchAtomics {
public:
  static void initialise();

  template <class T>
  constexpr static bool is_implemented()
  {
    return is_lock_free<T>();
  }

  template <class T>
  constexpr static bool is_lock_free()
  {
    return
      sizeof (T) == 1 ||
      sizeof (T) == 4;
  }

  template <class T>
  static T load(T& target)
  {
    fence();
    T ret = (volatile T&)target;
    fence();
    return ret;
  }

  template <class T>
  static void store(T& target, T value)
  {
    fence();
    ((volatile T&)target) = value;
    fence();
  }

  template <class T>
  static T exchange(T& target, T value) {
    return impl<T>::exchange(target, value);
  }

  template <class T>
  static bool compare_exchange(T& target, T& expected, T desired) {
    // armv5 cannot atomically compare and exchange
    //
    // either we disable interrupts here or we lock _ALL_ atomic operations
    // locking all atomics is too much of a performance hit and will also break
    // SWEBs SpinLock class
    //
    // this will only ever work on single-core systems

    bool int_state = ArchInterrupts::disableInterrupts();
    bool ret;
    T actual = load(target);
    if (actual == expected) {
      target = desired;
      ret = true;
    } else {
      expected = actual;
      ret = false;
    }
    if (int_state) ArchInterrupts::enableInterrupts();
    return ret;
  }

  template <class T>
  static T fetch_add(T& target, T inc) {
    T t = load(target);
    while (!compare_exchange(target, t, t + inc));
    return t;
  }

  template <class T>
  static T fetch_and(T& target, T mask)
  {
    T t = load(target);
    while (!compare_exchange(target, t, t & mask));
    return t;
  }

  template <class T>
  static T fetch_or(T& target, T mask)
  {
    T t = load(target);
    while (!compare_exchange(target, t, t | mask));
    return t;
  }

  template <class T>
  static T fetch_xor(T& target, T mask)
  {
    T t = load(target);
    while (!compare_exchange(target, t, t ^ mask));
    return t;
  }

  template <class T>
  static T fetch_sub(T& target, T dec)
  {
    return fetch_add(target, -dec);
  }

  template <class T>
  static T add_fetch(T& target, T inc)
  {
    return fetch_add(target, inc) + inc;
  }

  template <class T>
  static T sub_fetch(T& target, T dec)
  {
    return fetch_sub(target, dec) - dec;
  }

  template <class T>
  static T and_fetch(T& target, T mask)
  {
    return fetch_and(target, mask) & mask;
  }

  template <class T>
  static T or_fetch(T& target, T mask)
  {
    return fetch_or(target, mask) | mask;
  }

  template <class T>
  static T xor_fetch(T& target, T mask)
  {
    return fetch_xor(target, mask) ^ mask;
  }

  static size_t test_set_lock(size_t& target, size_t new_value);

  static void fence();

private:
  template <class T, size_t sz = sizeof (T)>
  class impl;
};

template <class T>
class ArchAtomics::impl<T, 1> {
public:
  static T exchange(T& target, T value) {
    T ret;
    asm(
      "swpb %0, %1, %2\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target)
    );
    return ret;
  }
};

template <class T>
class ArchAtomics::impl<T, 4> {
public:
  static T exchange(T& target, T value) {
    T ret;
    asm(
      "swp %0, %1, %2\n\t"
      : "=&r"(ret) : "r"(value), "Q"(target)
    );
    return ret;
  }
};
