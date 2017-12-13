/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_INCLUDE_SCRIPTING_SHEXCEPT_H_
#define MYSQLSHDK_INCLUDE_SCRIPTING_SHEXCEPT_H_

#include <stdexcept>
#include <string>

namespace shcore {

/** Invalid input from the user
 *
 * Sample uses:
 * - user input validation errors
 */
class user_input_error : public std::runtime_error {
 public:
  user_input_error(const char *what, const char *parameter);
  explicit user_input_error(const char *what);

 private:
  std::string param_;
};

/** Operation cancelled by the user
 *
 * Sample uses:
 * - User hit ^C
 **/
class cancelled : public std::runtime_error {
 public:
  explicit cancelled(const char *what);
};

/** Errors caused by an error in the user's script
 *
 * Sample uses:
 * - Python or JS script/snippet executed by the user has some error in its
 *  (like a syntax error or something)
 *
 * Usually scripting errors are supposed to be caught inside the script.
 */
class script_error : public std::runtime_error {
 public:
  explicit script_error(const char *what);
};
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SCRIPTING_SHEXCEPT_H_
