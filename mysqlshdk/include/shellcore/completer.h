/*
 * Copyright (c) 2017, 2022, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_INCLUDE_SHELLCORE_COMPLETER_H_
#define MYSQLSHDK_INCLUDE_SHELLCORE_COMPLETER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "shellcore/ishell_core.h"

namespace shcore {
namespace completer {
using Completion_list = std::vector<std::string>;

class Provider {
 public:
  using Provider_list = shcore::completer::Completion_list;

  virtual Completion_list complete(const std::string &buffer,
                                   const std::string &line,
                                   size_t *compl_offset) = 0;
  virtual ~Provider() = default;
};

class Completer {
 public:
  Completion_list complete(IShell_core::Mode mode, const std::string &buffer,
                           const std::string &line, size_t *compl_offset);

  void add_provider(IShell_core::Mode_mask mode_mask,
                    std::shared_ptr<Provider> provider,
                    bool before_all = false);

  void reset();

 private:
  std::vector<std::pair<IShell_core::Mode_mask, std::shared_ptr<Provider>>>
      providers_;
};

}  // namespace completer
}  // namespace shcore

#endif  // MYSQLSHDK_INCLUDE_SHELLCORE_COMPLETER_H_
