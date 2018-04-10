/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __mysh__utils_help__
#define __mysh__utils_help__

#include <string>
#include <vector>
#include "scripting/common.h"
#include "scripting/types_cpp.h"

namespace shcore {
class SHCORE_PUBLIC Shell_help {
 public:
  virtual ~Shell_help() {}

  // Retrieves the options directly, to be used from C++
  static Shell_help *get();

  std::string get_token(const std::string &help);

  void add_help(const std::string &token, const std::string &data);

 private:
  // Private constructor since this is a singleton
  Shell_help() {}

  // Options will be stored on a MAP
  std::map<std::string, std::string> _help_data;

  // The only available instance
  static Shell_help *_instance;
};

struct Help_register {
  Help_register(const std::string &token, const std::string &data);
};

std::vector<std::string> SHCORE_PUBLIC resolve_help_text(
    const std::vector<std::string> &prefixes, const std::string &suffix);
std::vector<std::string> SHCORE_PUBLIC get_help_text(const std::string &token);
std::string get_function_help(shcore::NamingStyle style,
                              const std::string &class_name,
                              const std::string &bfname);
std::string get_property_help(shcore::NamingStyle style,
                              const std::string &class_name,
                              const std::string &bfname);
std::string get_chained_function_help(shcore::NamingStyle style,
                                      const std::string &class_name,
                                      const std::string &bfname);
};  // namespace shcore

#define REGISTER_HELP(x, y) shcore::Help_register x(#x, y)

#endif /* defined(__mysh__utils_help__) */
