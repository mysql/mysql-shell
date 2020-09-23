/*
 * Copyright (c) 2020, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_LIBS_UTILS_NATURAL_COMPARE_H_
#define MYSQLSHDK_LIBS_UTILS_NATURAL_COMPARE_H_

#include <cctype>

namespace shcore {
namespace detail {
// based on Martin Pool's strnatcmp
template <typename InputIt1, typename InputIt2>
int compare_numbers(InputIt1 &first1, const InputIt1 &last1, InputIt2 &first2,
                    const InputIt2 &last2) {
  int bias = 0;
  for (; (first1 != last1) && (first2 != last2); first1++, first2++) {
    if (!::isdigit(*first1) && !::isdigit(*first2)) {
      return bias;
    } else if (!::isdigit(*first1)) {
      return -1;
    } else if (!::isdigit(*first2)) {
      return 1;
    } else if (*first1 < *first2) {
      if (bias == 0) {
        bias = -1;
      }
    } else if (*first2 < *first1) {
      if (bias == 0) {
        bias = 1;
      }
    }
  }

  return ((first1 == last1) && (first2 == last2)) ? bias : 0;
}

template <typename InputIt1, typename InputIt2>
int compare_fractional(InputIt1 &first1, const InputIt1 &last1,
                       InputIt2 &first2, const InputIt2 &last2) {
  for (; (first1 != last1) && (first2 != last2); first1++, first2++) {
    if (!::isdigit(*first1) && !::isdigit(*first2)) {
      return 0;
    } else if (!::isdigit(*first1)) {
      return -1;
    } else if (!::isdigit(*first2)) {
      return 1;
    } else if (*first1 < *first2) {
      return -1;
    } else if (*first2 < *first1) {
      return 1;
    }
  }
  return 0;
}
}  // namespace detail

template <typename InputIt1, typename InputIt2>
bool natural_compare(InputIt1 first1, InputIt1 last1, InputIt2 first2,
                     InputIt2 last2) {
  while ((first1 != last1) && (first2 != last2)) {
    if (::isdigit(*first1) && ::isdigit(*first2)) {
      int r = 0;
      if (*first1 == '0' || *first2 == '0') {
        r = detail::compare_fractional(first1, last1, first2, last2);
      } else {
        r = detail::compare_numbers(first1, last1, first2, last2);
      }
      if (r == -1) {
        return true;
      }
      if (r == 1) {
        return false;
      }
      continue;
    }
    if (*first1 < *first2) {
      return true;
    }
    if (*first2 < *first1) {
      return false;
    }
    ++first1;
    ++first2;
  }
  return (first1 == last1) && (first2 != last2);
}

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_NATURAL_COMPARE_H_
