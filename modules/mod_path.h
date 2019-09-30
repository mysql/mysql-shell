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

#include <string>

#include "scripting/types_cpp.h"

#ifndef MODULES_MOD_PATH_H_
#define MODULES_MOD_PATH_H_

namespace mysqlsh {

/**
 * \ingroup ShellAPI
 * $(PATH_BRIEF)
 */
class SHCORE_PUBLIC Path : public shcore::Cpp_object_bridge {
 public:
#if DOXYGEN_JS
  String basename(String path);
  String dirname(String path);
  Bool isabs(String path);
  Bool isdir(String path);
  Bool isfile(String path);
  String join(String pathA, String pathB, String pathC, String pathD);
  String normpath(String path);
#endif

  Path();

  std::string class_name() const override { return "Path"; };

 private:
  std::string basename(const std::string &path) const;
  std::string dirname(const std::string &path) const;
  bool isabs(const std::string &path) const;
  bool isdir(const std::string &path) const;
  bool isfile(const std::string &path) const;
  std::string join(const std::string &first, const std::string &second,
                   const std::string &third, const std::string &fourth) const;
  std::string normpath(const std::string &path) const;
};

}  // namespace mysqlsh

#endif  // MODULES_MOD_PATH_H_
