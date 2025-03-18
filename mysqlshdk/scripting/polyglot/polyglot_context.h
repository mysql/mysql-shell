/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_SCRIPTING_POLYGLOT_POLYGLOT_CONTEXT_H_
#define MYSQLSHDK_SCRIPTING_POLYGLOT_POLYGLOT_CONTEXT_H_

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/scripting/polyglot/shell_polyglot_common_context.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_store.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_utils.h"

namespace shcore {

class Object_registry;

namespace polyglot {
class Polyglot_language;

class Polyglot_context {
 public:
  Polyglot_context(Object_registry *registry, Language type);
  ~Polyglot_context();

  // ==== Shell Interface ====
  void set_argv(const std::vector<std::string> &args);

  void set_global(const std::string &name, const Value &value);
  Value get_global(const std::string &name);

  std::pair<Value, bool> debug(Object_registry *registry, Language type,
                               const std::string &path);

  std::pair<Value, bool> execute(const std::string &code,
                                 const std::string &source = "");
  std::pair<Value, bool> execute_interactive(const std::string &code,
                                             Input_state *r_state,
                                             bool flush = true) noexcept;

  void terminate();

  bool load_plugin(const Plugin_definition &plugin);

  // ==== Autocompletion interface ====
  std::tuple<polyglot::Store, std::string> get_global_polyglot(
      const std::string &name);

  std::tuple<bool, polyglot::Store, std::string> get_member_of(
      const poly_value obj, const std::string &name);

  std::vector<std::pair<bool, std::string>> get_members_of(
      const poly_value obj);
  std::vector<std::pair<bool, std::string>> list_globals();
  const std::vector<std::string> &keywords() const;

 private:
#ifdef FRIEND_TEST
  FRIEND_TEST(JavaScript, simple_to_js_and_back);
  FRIEND_TEST(JavaScript, array_to_js);
  FRIEND_TEST(JavaScript, map_to_js);
  FRIEND_TEST(JavaScript, object_to_js);
#endif

  std::shared_ptr<Polyglot_language> get_language(
      Language type, const std::string &debug_port = "",
      bool wait_attached = false);

  Value convert(poly_value value) const;
  poly_value convert(const Value &value) const;

  poly_context context() const;
  poly_thread thread() const;

  poly_value type_info(poly_value value) const;
  std::string type_name(poly_value value) const;

  Shell_polyglot_common_context m_common_context;
  std::shared_ptr<Polyglot_language> m_language;
};

}  // namespace polyglot
}  // namespace shcore

#endif  // MYSQLSHDK_SCRIPTING_POLYGLOT_POLYGLOT_CONTEXT_H_
