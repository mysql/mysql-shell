/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_checker/upgrade_check.h"

#include <sstream>
#include <stdexcept>
#include <string>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_translate.h"

namespace mysqlsh {
namespace upgrade_checker {

using mysqlshdk::utils::Version;

const char *Upgrade_check::get_text(const char *field) const {
  std::string path = shcore::get_share_folder();
  path = shcore::path::join_path(path, "upgrade_checker.msg");

  if (!shcore::path::exists(path))
    throw std::runtime_error(path +
                             ": not found, shell installation likely invalid");

  static shcore::Translation translation =
      shcore::read_translation_from_file(path.c_str());
  std::stringstream str;
  str << m_name << "." << field;
  auto it = translation.find(str.str());
  if (it == translation.end()) return nullptr;
  return it->second.c_str();
}

const char *Upgrade_check::get_title() const {
  const char *translated = get_text("title");
  if (translated) return translated;
  return get_title_internal();
}

const char *Upgrade_check::get_description() const {
  const char *translated = get_text("description");
  if (translated) return translated;
  return get_description_internal();
}

const char *Upgrade_check::get_doc_link() const {
  const char *translated = get_text("docLink");
  if (translated) return translated;
  return get_doc_link_internal();
}

}  // namespace upgrade_checker
}  // namespace mysqlsh