/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

#include <string>
#include <type_traits>

#include "mysqlshdk/libs/utils/nullable.h"
#include "mysqlshdk/libs/utils/std.h"

namespace mysqlshdk {
namespace utils {

/**
 * Helper class which is designed to be a drop-in replacement for the wrapped
 * type. Stores two values: the real one, and optionally a masked one. If the
 * masked value is given, the real one is considered unsafe (i.e. to be logged),
 * and it can only be accessed explicitly via real() method, while implicit
 * conversion returns the masked value. If the masked value is not given, then
 * the real one is "safe" and both real() method and implicit conversion return
 * the real value.
 *
 * If the real value is given as lvalue reference, value is not copied, but
 * reference is stored instead.
 */
template <typename T>
class Masked_value final {
 public:
  Masked_value() = delete;

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
            std::enable_if_t<!std::is_same<Masked_value<T>,
                                           std20::remove_cvref_t<U>>::value &&
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

  Masked_value(const Masked_value &v) = default;
  Masked_value(Masked_value &&v) = default;

  Masked_value &operator=(const Masked_value &v) = default;
  Masked_value &operator=(Masked_value &&v) = default;

  ~Masked_value() = default;

  const T &real() const { return m_real_ref; }

  const T &masked() const { return m_masked.get_safe(real()); }

  /**
   * Implicit conversion, uses the masked value.
   */
  operator const T &() const { return masked(); }

 private:
  T m_real;

  const T &m_real_ref;

  nullable<T> m_masked;

#ifdef FRIEND_TEST
  FRIEND_TEST(Masked_value_test, constructors);
#endif
};

}  // namespace utils

using Masked_string = utils::Masked_value<std::string>;

}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_UTILS_MASKED_VALUE_H_
