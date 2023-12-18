/*
 * Copyright (c) 2021, 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_LIBS_UTILS_MASKED_VALUE_H_
#define MYSQLSHDK_LIBS_UTILS_MASKED_VALUE_H_

#include <functional>
#include <optional>
#include <string>
#include <type_traits>

namespace mysqlshdk {
namespace utils {

/**
 * Stores two values: the real one, and optionally a masked one. If the masked
 * value is given, the real one is considered unsafe (i.e. to be logged),
 * and it can only be accessed via real() method. If the masked value is not
 * given, then the real one is "safe" and both real() and masked() methods
 * return the real value.
 *
 * If the real value is given as lvalue reference, value is not copied, but
 * reference is stored instead.
 */
template <typename T>
class Masked_value final {
 public:
  Masked_value() : Masked_value("") {}

  /**
   * Implicit conversion, masked and real values are the same.
   *
   * Perfect forwarding constructor, disabled if U resolves to this type, so
   * that this constructor is not selected instead of a copy constructor.
   *
   * Disabled if a reference to T is passed, in order to avoid copy (the other
   * constructor is selected).
   */
  template <typename U,
            std::enable_if_t<
                !std::is_same<Masked_value<T>, std::remove_cvref_t<U>>::value &&
                    !std::is_same<T &, std::remove_cv_t<U>>::value,
                int> = 0>
  Masked_value(U &&real) : m_real(std::forward<U>(real)), m_real_ref(m_real) {}

  /**
   * Stores the reference to the real value (copy is not performed).
   */
  Masked_value(const T &real) : m_real_ref(real) {}

  /**
   * Creates both real and masked values in place.
   *
   * Disabled if a reference to T is passed, in order to avoid copy (the other
   * constructor is selected).
   */
  template <
      typename U, typename V,
      std::enable_if_t<!std::is_same<T &, std::remove_cv_t<U>>::value, int> = 0>
  Masked_value(U &&real, V &&masked)
      : m_real(std::forward<U>(real)),
        m_real_ref(m_real),
        m_masked(std::forward<V>(masked)) {}

  /**
   * Stores the reference to the real value (copy is not performed), masked
   * value is created in place.
   */
  template <typename V>
  Masked_value(const T &real, V &&masked)
      : m_real_ref(real), m_masked(std::forward<V>(masked)) {}

  Masked_value(const Masked_value &v) : m_real_ref(m_real) { *this = v; }

  Masked_value(Masked_value &&v) noexcept : m_real_ref(m_real) {
    *this = std::move(v);
  }

  Masked_value &operator=(const Masked_value &v) {
    if (&v.m_real == &v.m_real_ref.get()) {
      // reference points to the v.m_real, need to copy the value
      m_real = v.m_real;
      m_real_ref = m_real;
    } else {
      // reference points to a value stored somewhere else, copy the reference
      m_real = {};
      m_real_ref = v.m_real_ref;
    }

    m_masked = v.m_masked;

    return *this;
  }

  Masked_value &operator=(Masked_value &&v) noexcept {
    if (&v.m_real == &v.m_real_ref.get()) {
      // reference points to the v.m_real, need to move the value
      m_real = std::move(v.m_real);
      m_real_ref = m_real;
    } else {
      // reference points to a value stored somewhere else, move the reference
      m_real = {};
      m_real_ref = std::move(v.m_real_ref);
    }

    m_masked = std::move(v.m_masked);

    return *this;
  }

  ~Masked_value() = default;

  const T &real() const { return m_real_ref; }

  T masked() const { return m_masked.value_or(real()); }

  bool operator==(const Masked_value &mv) const {
    // call the other comparison operator
    return *this == mv.real();
  }

  /**
   * Compares the given value with the real value.
   */
  bool operator==(const T &v) const { return real() == v; }

 private:
  T m_real;

  std::reference_wrapper<const T> m_real_ref;

  std::optional<T> m_masked;

#ifdef FRIEND_TEST
  FRIEND_TEST(Masked_value_test, constructors);
#endif
};

template <typename T>
bool operator==(const T &v, const Masked_value<T> &mv) {
  // call the comparison operator from Masked_value class
  return mv == v;
}

}  // namespace utils

using Masked_string = utils::Masked_value<std::string>;

}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_MASKED_VALUE_H_
