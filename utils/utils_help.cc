/*
* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "utils_help.h"

using namespace shcore;

Shell_help *Shell_help::_instance = nullptr;

Shell_help *Shell_help::get() {
  if (!_instance)
    _instance = new Shell_help();

  return _instance;
}

void Shell_help::add_help(const std::string& token, const std::string& data) {
  _help_data[token] = data;
}

std::string Shell_help::get_token(const std::string& token) {
  std::string ret_val;

  if (_help_data.find(token) != _help_data.end())
      ret_val = _help_data[token];

  return ret_val;
}

Help_register::Help_register(const std::string &token, const std::string &data) {
  shcore::Shell_help::get()->add_help(token, data);
};
