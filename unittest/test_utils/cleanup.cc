/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "unittest/test_utils/cleanup.h"

#include <string>
#include <utility>

#include "mysqlshdk/libs/utils/utils_general.h"

namespace tests {

Cleanup::~Cleanup() {
  while (!m_steps.empty()) {
    m_steps.back()();
    m_steps.pop_back();
  }
}

Cleanup &Cleanup::add(Step step) {
  m_steps.emplace_back(std::move(step));
  return *this;
}

Cleanup &Cleanup::add(Cleanup c) {
  for (auto &s : c.m_steps) {
    add(std::move(s));
  }

  c.m_steps.clear();

  return *this;
}

Cleanup &Cleanup::operator+=(Cleanup c) { return add(std::move(c)); }

[[nodiscard]] Cleanup Cleanup::unset_env_var(const char *name) {
  Cleanup c;

  if (const auto v = ::getenv(name)) {
    c.add([name, old_value = std::string{v}]() {
      shcore::setenv(name, old_value);
    });

    shcore::unsetenv(name);
  }

  return c;
}

[[nodiscard]] Cleanup Cleanup::set_env_var(const char *name,
                                           const std::string &value) {
  Cleanup c;

  c += unset_env_var(name);

  shcore::setenv(name, value);
  c.add([name]() { shcore::unsetenv(name); });

  return c;
}

}  // namespace tests
