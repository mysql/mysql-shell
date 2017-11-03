/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_MOD_SHELL_OPTIONS_H_
#define MODULES_MOD_SHELL_OPTIONS_H_

#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "scripting/types_cpp.h"

#define SN_SHELL_OPTION_CHANGED "SN_SHELL_OPTION_CHANGED"

namespace shcore {
class SHCORE_PUBLIC Mod_shell_options : public shcore::Cpp_object_bridge {
 public:
  explicit Mod_shell_options(std::shared_ptr<mysqlsh::Shell_options> options);
  virtual ~Mod_shell_options() {
  }

  // Exposes the object to JS/PY to allow custom validatiogit ns on options
  static std::shared_ptr<Mod_shell_options> get_instance();
  static void reset_instance();

  std::string class_name() const override {
    return "Shell Object";
  }

  bool operator==(const Object_bridge &other) const override;
  std::vector<std::string> get_members() const override {
    return options->get_named_options();
  }
  Value get_member(const std::string &prop) const override;
  bool has_member(const std::string &prop) const override {
    return options->has_key(prop);
  }
  void set_member(const std::string &prop, Value value) override;
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;

 private:
  std::shared_ptr<mysqlsh::Shell_options> options;
};
}  // namespace shcore

#endif  // MODULES_MOD_SHELL_OPTIONS_H_
