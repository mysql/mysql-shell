/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_ERROR_H_
#define MYSQLSHDK_LIBS_UTILS_ERROR_H_

#include <stdexcept>
#include <string>

namespace shcore {

class Error : public std::runtime_error {
 public:
  Error(const char *what, int code) : std::runtime_error(what), code_(code) {}

  Error(const std::string &what, int code)
      : std::runtime_error(what), code_(code) {}

  virtual ~Error() {}

  int code() const noexcept { return code_; }

  virtual std::string format() const {
    if (code() != -1)
      return "Error " + std::to_string(code()) + ": " + what();
    else
      return what();
  }

 protected:
  int code_ = -1;
};

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_ERROR_H_
