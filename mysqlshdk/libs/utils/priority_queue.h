/*
 * Copyright (c) 2023, 2024, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MYSQLSHDK_LIBS_UTILS_PRIORITY_QUEUE_H_
#define MYSQLSHDK_LIBS_UTILS_PRIORITY_QUEUE_H_

#include <algorithm>
#include <queue>
#include <stdexcept>

namespace shcore {

/**
 * A priority queue which can handle std::unique_ptr.
 */
template <class T, class Container = std::vector<T>,
          class Compare = std::less<typename Container::value_type>>
class Priority_queue : public std::priority_queue<T, Container, Compare> {
 public:
  using std::priority_queue<T, Container, Compare>::priority_queue;

  T pop_top() {
    if (this->c.empty()) {
      throw std::runtime_error("Trying to pop from an empty queue.");
    }

    std::pop_heap(this->c.begin(), this->c.end(), this->comp);
    auto v = std::move(this->c.back());
    this->c.pop_back();
    return v;
  }
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_PRIORITY_QUEUE_H_
