/*
* Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; version 2 of the
* License.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301  USA
*/

#ifndef __mysh__utils_help__
#define __mysh__utils_help__

#include "shellcore/common.h"
#include <string>
#include <vector>

namespace shcore {
class SHCORE_PUBLIC  Shell_help {
public:
  virtual ~Shell_help() {};

  // Retrieves the options directly, to be used from C++
  static Shell_help* get();

  std::string get_token(const std::string& help);

  void add_help(const std::string& token, const std::string& data);

private:
  // Private constructor since this is a singleton
  Shell_help() {};

  // Options will be stored on a MAP
  std::map<std::string, std::string> _help_data;

  // The only available instance
  static Shell_help* _instance;
};

struct Help_register {
  Help_register(const std::string &token, const std::string &data);
};

std::vector<std::string> SHCORE_PUBLIC get_help_text(const std::string& token, const std::vector<size_t> &sizes = {});
};

#define REGISTER_HELP(x,y) shcore::Help_register x(#x,y)

#endif /* defined(__mysh__utils_help__) */
