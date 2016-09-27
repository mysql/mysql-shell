/*
* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "utils_general.h"
#include <vector>
#include <cctype>

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

std::vector<std::string> shcore::get_help_text(const std::string& token) {
  std::string real_token;
  for (auto c : token)
    real_token.append(1, std::toupper(c));

  int index = 0;
  std::string text = Shell_help::get()->get_token(real_token);

  std::vector<std::string> lines;
  while (!text.empty()) {
    lines.push_back(text);

    text = Shell_help::get()->get_token(real_token + std::to_string(++index));
  }

  return lines;
}
