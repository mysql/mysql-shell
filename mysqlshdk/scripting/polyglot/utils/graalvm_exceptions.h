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

#ifndef MYSQLSHDK_SCRIPTING_POLYGLOT_UTILS_POLYGLOT_EXCEPTIONS_H_
#define MYSQLSHDK_SCRIPTING_POLYGLOT_UTILS_POLYGLOT_EXCEPTIONS_H_

#include <stdexcept>
#include <string>

namespace shcore {
namespace polyglot {

class Graalvm_exception : public std::runtime_error {
 public:
  const char *id() const { return m_id; }

 protected:
  Graalvm_exception(const char *id, const char *msg)
      : std::runtime_error(msg), m_id{id} {}

 private:
  const char *m_id;
};

class Unsupported_operation_exception : public Graalvm_exception {
 public:
  Unsupported_operation_exception(const char *msg)
      : Graalvm_exception("UnsupportedOperationException", msg) {}
};

}  // namespace polyglot
}  // namespace shcore

#endif  // MYSQLSHDK_SCRIPTING_POLYGLOT_UTILS_POLYGLOT_EXCEPTIONS_H_