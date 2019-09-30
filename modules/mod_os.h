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

#include <memory>
#include <string>

#include "scripting/types_cpp.h"

#ifndef MODULES_MOD_OS_H_
#define MODULES_MOD_OS_H_

namespace mysqlsh {

class Path;

/**
 * \ingroup ShellAPI
 * $(OS_BRIEF)
 */
class SHCORE_PUBLIC Os : public shcore::Cpp_object_bridge {
 public:
#if DOXYGEN_JS
  Path path;
  String getcwd();
  String getenv(String name);
  String loadTextFile(String path);
  Undefined sleep(Number seconds);
  Bool file_exists(String path);
  String get_binary_folder();
  String get_mysqlx_home_path();
  String get_user_config_path();
  String load_text_file(String path);
#endif

  Os();

  std::string class_name() const override { return "Os"; };

  shcore::Value get_member(const std::string &prop) const override;

 private:
  shcore::Value getenv(const std::string &name) const;
  std::string getcwd() const;
  void sleep(double sec) const;
  std::string load_text_file(const std::string &path) const;
  std::string get_user_config_path() const;
  std::string get_mysqlx_home_path() const;
  std::string get_binary_folder() const;
  bool file_exists(const std::string &path) const;
  std::string load_text_file_(const std::string &path) const;

  std::shared_ptr<Path> m_path;
};

}  // namespace mysqlsh

#endif  // MODULES_MOD_OS_H_
