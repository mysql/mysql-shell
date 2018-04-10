/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "shellcore/completer.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <set>

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace shcore {
namespace completer {
Completion_list Completer::complete(IShell_core::Mode mode,
                                    const std::string &text,
                                    size_t *compl_offset) {
  Completion_list list;
  for (auto &iter : providers_) {
    if (iter.first.is_set(mode)) {
      size_t old_compl_offs = *compl_offset;
      auto tmp = iter.second->complete(text, compl_offset);
      if (tmp.empty()) {
        *compl_offset = old_compl_offs;
      } else {
        std::copy(tmp.begin(), tmp.end(), std::back_inserter(list));
        if (old_compl_offs != *compl_offset) break;
      }
    }
  }
  return list;
}

void Completer::add_provider(IShell_core::Mode_mask mode_mask,
                             std::shared_ptr<Provider> provider,
                             bool before_all) {
  if (before_all) {
    providers_.insert(providers_.begin(), {mode_mask, provider});
  } else {
    providers_.push_back({mode_mask, provider});
  }
}

void Completer::reset() { providers_.clear(); }

}  // namespace completer
}  // namespace shcore
