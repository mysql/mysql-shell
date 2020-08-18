/*
 * Copyright (c) 2017, 2020, Oracle and/or its affiliates.
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
#include "utils/utils_json.h"

namespace mysqlsh {

/**
 * \ingroup ShellAPI
 *
 * @brief $(OPTIONS_BRIEF)
 *
 * $(OPTIONS_DETAIL)
 * $(OPTIONS_DETAIL1)
 * $(OPTIONS_DETAIL2)
 * $(OPTIONS_DETAIL3)
 * $(OPTIONS_DETAIL4)
 * $(OPTIONS_DETAIL5)
 * $(OPTIONS_DETAIL6)
 * $(OPTIONS_DETAIL7)
 * $(OPTIONS_DETAIL8)
 * $(OPTIONS_DETAIL9)
 * $(OPTIONS_DETAIL10)
 * $(OPTIONS_DETAIL11)
 * $(OPTIONS_DETAIL12)
 * $(OPTIONS_DETAIL13)
 * $(OPTIONS_DETAIL14)
 * $(OPTIONS_DETAIL15)
 * $(OPTIONS_DETAIL16)
 * $(OPTIONS_DETAIL17)
 * $(OPTIONS_DETAIL18)
 * $(OPTIONS_DETAIL19)
 *
 * $(OPTIONS_DETAIL20)
 * $(OPTIONS_DETAIL21)
 * $(OPTIONS_DETAIL22)
 * $(OPTIONS_DETAIL23)
 * $(OPTIONS_DETAIL24)
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
  shcore::Value get_member(const std::string &prop) const override;
  void set_member(const std::string &prop, shcore::Value value) override;
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;

  void append_json(shcore::JSON_dumper &dumper) const override;

#if DOXYGEN_JS
  Undefined set(String optionName, Value value);
  Undefined setPersist(String optionName, Value value);
  Undefined unset(String optionName);
  Undefined unsetPersist(String optionName);
#elif DOXYGEN_PY
  None set(str optionName, value value);
  None set_persist(str optionName, value value);
  None unset(str optionName);
  None unset_persist(str optionName);
#endif

  void set(const std::string &option_name, const shcore::Value &value);
  void set_persist(const std::string &option_name, const shcore::Value &value);
  void unset(const std::string &option_name);
  void unset_persist(const std::string &option_name);

 private:
  std::shared_ptr<mysqlsh::Shell_options> shell_options;
};
}  // namespace mysqlsh

#endif  // MODULES_MOD_SHELL_OPTIONS_H_
