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

#ifndef MYSQLSHDK_SHELLCORE_PROVIDER_JAVASCRIPT_H_
#define MYSQLSHDK_SHELLCORE_PROVIDER_JAVASCRIPT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/shellcore/provider_script.h"
#include "scripting/jscript_context.h"

namespace shcore {
namespace completer {

class Provider_javascript : public Provider_script {
 public:
  Provider_javascript(std::shared_ptr<Object_registry> registry,
                      std::shared_ptr<JScript_context> context);

 private:
  friend class JavaScript_proxy;

  std::shared_ptr<JScript_context> context_;

  Completion_list complete_chain(const Chain &chain) override;

  Chain parse_until(const std::string &s, size_t *pos, int close_char,
                    size_t *chain_start_pos) override;

  std::shared_ptr<Object> lookup_global_object(
      const std::string &name) override;
};

}  // namespace completer
}  // namespace shcore

#endif  // MYSQLSHDK_SHELLCORE_PROVIDER_JAVASCRIPT_H_
