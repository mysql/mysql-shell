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

#ifndef MODULES_MOD_SHELL_OPTIONS_H_
#define MODULES_MOD_SHELL_OPTIONS_H_

#include <memory>
#include <string>
#include <vector>
#include "mysqlshdk/include/shellcore/shell_options.h"
#include "scripting/types_cpp.h"

namespace mysqlsh {

/**
 * \ingroup ShellAPI
 * $(OPTIONS_BRIEF)
 */
class SHCORE_PUBLIC Options : public shcore::Cpp_object_bridge {
 public:
  explicit Options(std::shared_ptr<mysqlsh::Shell_options> options);
  virtual ~Options() {}

  // Exposes the object to JS/PY to allow custom validations on options
  static std::shared_ptr<Options> get_instance();
  static void reset_instance();

  std::string class_name() const override { return "Options"; }

  bool operator==(const Object_bridge &other) const override;
  std::vector<std::string> get_members() const override {
    return shell_options->get_named_options();
  }
  shcore::Value get_member(const std::string &prop) const override;
  bool has_member(const std::string &prop) const override {
    return shell_options->has_key(prop);
  }
  void set_member(const std::string &prop, shcore::Value value) override;
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;

#if DOXYGEN_JS
  Undefined set(String optionName, Value value);
  Undefined set_persist(String optionName, Value value);
  Undefined unset(String optionName);
  Undefined unset_persist(String optionName);
#elif DOXYGEN_PY
  None set(str optionName, value value);
  None set_persist(str optionName, value value);
  None unset(str optionName);
  None unset_persist(str optionName);
#endif

  shcore::Value set(const shcore::Argument_list &args);
  shcore::Value set_persist(const shcore::Argument_list &args);
  shcore::Value unset(const shcore::Argument_list &args);
  shcore::Value unset_persist(const shcore::Argument_list &args);

 private:
  std::shared_ptr<mysqlsh::Shell_options> shell_options;
};
}  // namespace mysqlsh

#endif  // MODULES_MOD_SHELL_OPTIONS_H_
