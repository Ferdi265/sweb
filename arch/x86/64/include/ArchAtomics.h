#pragma once

#include "types.h"

class ArchAtomics {
public:
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
      sizeof (T) == 2 ||
      sizeof (T) == 4 ||
      sizeof (T) == 8;
  }

  template <class T>
  static T load(T& target)
  {
    T ret;
    fence();
    __asm__ __volatile__(
      "mov%z0 %1, %0\n\t"
      : "=r"(ret) : "m"(target)
    );
    return ret;
  }

  template <class T>
  static void store(T& target, T value)
  {
    __asm__ __volatile__(
      "mov%z0 %0, %1\n\t"
      :: "r"(value), "m"(target)
    );
    fence();
  }

  template <class T>
  static T exchange(T& target, T val)
  {
    __asm__ __volatile__(
      "lock xchg%z0 %0, %1\n\t"
      : "+r"(val) : "m"(target)
    );
    return val;
  }

  template <class T>
  static bool compare_exchange(T& target, T& expected, T desired)
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
  static T fetch_add(T& target, T inc)
  {
    __asm__ __volatile__(
      "lock xadd%z0 %0, %1\n\t"
      : "+r"(inc) : "m"(target)
    );
    return inc;
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

  static void fence()
  {
    asm("mfence");
  }

  static size_t test_set_lock(size_t& lock, size_t new_value)
  {
    return exchange<size_t>(lock, new_value);
  }
};
