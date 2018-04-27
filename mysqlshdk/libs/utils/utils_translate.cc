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

#include "mysqlshdk/libs/utils/utils_translate.h"
#include <errno.h>
#include <string.h>
#include <stdexcept>
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {

Translation_writer::Translation_writer(const char *filename,
                                       int max_line_length)
    : m_file(filename, std::ofstream::binary),
      m_max_line_length(max_line_length) {
  if (!m_file.is_open())
    throw std::runtime_error(std::string("Unable to open file ") + filename +
                             " for writing: " + strerror(errno));
}

void Translation_writer::write_header(const char *custom_text) {
  if (!custom_text) {
    custom_text =
        "This file defines translations one by one (comments can be added for "
        "readability starting with '#' character). Format for single "
        "translation is as follows:\n\n"
        "-------------------------------------------------------------------\n"
        "* Translation id: one or more lines (if id contains new line "
        "characters)\n\n"
        "# Zero or more lines: text - originally given in source code with "
        "formatting suggestions\n\n"
        "One or more lines - translation. Line breaks will be ignored and can "
        "be put in translations for readability - if line break is intended "
        "deliberately, line must end with '\\n' character sequence.\n\n"
        "Empty line to terminate single translation\n"
        "-------------------------------------------------------------------";
  }
  auto lines = str_break_into_lines(custom_text, m_max_line_length - 2);
  for (const auto &line : lines)
    if (line.empty())
      m_file << "#\n";
    else
      m_file << "# " << line << '\n';
  m_file << std::endl;
}

void Translation_writer::write_entry(const char *id, const char *entry_format,
                                     const char *initial_text) {
  if (id != nullptr && strlen(id) > 0) {
    auto lines = str_break_into_lines(str_replace(id, "\n", "\\n\n"),
                                      m_max_line_length - 2);
    for (const auto &line : lines) m_file << "* " << line << '\n';
  } else {
    throw std::invalid_argument("Translation id cannot be empty");
  }

  if (entry_format != nullptr && strlen(entry_format) > 0) {
    auto lines = str_break_into_lines(str_replace(entry_format, "\n", "\\n\n"),
                                      m_max_line_length - 2);
    for (const auto &line : lines) m_file << "# " << line << '\n';
  }

  if (initial_text != nullptr && strlen(initial_text) > 0) {
    auto lines = str_break_into_lines(str_replace(initial_text, "\n", "\\n\n"),
                                      m_max_line_length);
    for (const auto &line : lines) m_file << line << '\n';
  }

  m_file << std::endl;
}

static inline std::string fix_trailing_nl(const std::string &str) {
  if (str.length() < 2 || str[str.length() - 2] != '\\' || str.back() != 'n')
    return str + " ";
  return str.substr(0, str.length() - 2) + "\n";
}

Translation read_translation_from_file(const char *filename) {
  Translation result;
  std::ifstream file(filename, std::ifstream::binary);
  if (!file.is_open())
    throw std::runtime_error(std::string("Unable to open file ") + filename +
                             " for reading: " + strerror(errno));

  std::string line;
  std::size_t i = 0;

  while (++i && std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;

    if (line[0] != '*' || line.length() < 3)
      throw std::runtime_error("Translation file malformated line:" +
                               std::to_string(i));

    std::string id = fix_trailing_nl(line.substr(2));
    while (++i && std::getline(file, line) && !line.empty() && line[0] == '*')
      if (line.length() < 3)
        throw std::runtime_error("Translation file malformated line:" +
                                 std::to_string(i));
      else
        id += fix_trailing_nl(line.substr(2));

    if (line.empty()) continue;

    std::string translation;
    do {
      if (line[0] == '#') continue;
      if (line[0] == '*')
        throw std::runtime_error(
            "Translation file malformated (empty line required before new "
            "translation id) line:" +
            std::to_string(i));
      translation += fix_trailing_nl(line);
    } while (++i && std::getline(file, line) && !line.empty());

    if (!translation.empty() &&
        !result
             .emplace(id.back() == ' ' ? id.substr(0, id.length() - 1) : id,
                      translation.back() == ' '
                          ? translation.substr(0, translation.length() - 1)
                          : translation)
             .second)
      throw std::runtime_error(
          "Translation file malformated line, found not unique id:" + id);
  }

  return result;
}

}  // namespace shcore
