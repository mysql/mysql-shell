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

#ifndef UNITTEST_TEST_UTILS_TEST_COMBO_UTILITIES_H_
#define UNITTEST_TEST_UTILS_TEST_COMBO_UTILITIES_H_

#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/utils/utils_string.h"

namespace tests {

template <typename T>
class Combination_iterator {
 public:
  void add_variable(const std::string &name, const std::vector<T> &values) {
    assert(!values.empty());
    Variable var{name, values, 0};
    m_variables.emplace_back(std::move(var));
    m_values.emplace_back(name, values.front());
  }

  bool next(const std::vector<std::pair<std::string, T>> **out_values) {
    auto val = m_values.rbegin();

    for (auto v = m_variables.rbegin(); v != m_variables.rend(); ++v, ++val) {
      if (v->value_i == v->values.size() - 1) {
        // if this is the 1st variable, we're done
        if (v == m_variables.rend() - 1) return false;
        v->value_i = 0;
        val->second = v->values[v->value_i];
      } else {
        v->value_i++;
        val->second = v->values[v->value_i];
        break;
      }
    }

    *out_values = &values();
    return true;
  }

  T get(const std::string &name) const {
    for (const auto &v : m_values) {
      if (v.first == name) return v.second;
    }
    throw std::invalid_argument("Invalid variable " + name);
  }

  const std::vector<std::pair<std::string, T>> &values() const {
    return m_values;
  }

 private:
  struct Variable {
    std::string name;
    std::vector<T> values;
    size_t value_i;
  };

  std::vector<Variable> m_variables;
  std::vector<std::pair<std::string, T>> m_values;
  size_t variable_i = 0;
};

class String_combinator : Combination_iterator<std::string> {
 public:
  explicit String_combinator(const std::string &templ) : m_templ(templ) {}

  void add(const std::string &name, const std::vector<std::string> &values) {
    add_variable(name, values);
  }

  std::string get() {
    std::string str = m_templ;

    const std::vector<std::pair<std::string, std::string>> *values;

    if (!next(&values)) {
      return "";
    }

    for (const auto &var : *values) {
      str = shcore::str_replace(str, "{" + var.first + "}", var.second);
    }

    return str;
  }

  std::string get(const std::string &name) {
    return Combination_iterator<std::string>::get(name);
  }

 private:
  std::string m_templ;
};

}  // namespace tests

#endif  // UNITTEST_TEST_UTILS_TEST_COMBO_UTILITIES_H_
