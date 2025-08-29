/*
 * Copyright (c) 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/textui/progress_attribs.h"

namespace mysqlshdk {
namespace textui {

void Progress_attributes::set_extra_attributes(const Attributes &attribs) {
  if (!attribs) {
    return;
  }

  if (!m_extra_attribs) {
    m_extra_attribs = shcore::make_dict();
  }

  for (auto &attribute : *attribs) {
    m_extra_attribs->set(attribute.first, attribute.second);
  }
}

Attributes format_json_throughput(uint64_t items, double seconds) {
  auto throughput = shcore::make_dict();

  throughput->emplace("items", items);
  throughput->emplace("seconds", seconds);

  return throughput;
}

}  // namespace textui
}  // namespace mysqlshdk
