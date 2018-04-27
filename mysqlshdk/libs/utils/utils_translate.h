/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_UTILS_UTILS_TRANSLATE_H_
#define MYSQLSHDK_LIBS_UTILS_UTILS_TRANSLATE_H_

#include <fstream>
#include <string>
#include <unordered_set>
#include <unordered_map>

namespace shcore {

class Translation_writer {
 public:
  explicit Translation_writer(const char* filename, int max_line_length = 100);

  void write_header(const char* custom_text = nullptr);

  void write_entry(const char *id, const char *entry_format = nullptr,
                 const char *initial_text = nullptr);

 private:
  std::ofstream m_file;
  std::unordered_set<std::string> m_idset;
  int m_max_line_length;
};

using Translation = std::unordered_map<std::string, std::string>;

Translation read_translation_from_file(const char* filename);

}  // namespace shcore

#endif  // MYSQLSHDK_LIBS_UTILS_UTILS_TRANSLATE_H_
