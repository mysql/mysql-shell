/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#ifndef MYSQLSHDK_SCRIPTING_POLYGLOT_SHELL_JAVASCRIPT_H_
#define MYSQLSHDK_SCRIPTING_POLYGLOT_SHELL_JAVASCRIPT_H_

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "mysqlshdk/include/scripting/lang_base.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/scripting/polyglot/languages/polyglot_javascript.h"
#include "mysqlshdk/scripting/polyglot/shell_polyglot_common_context.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_store.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_utils.h"

namespace shcore {

class Object_registry;

namespace polyglot {

#ifdef HAVE_JS
/**
 * This class is an extension to the default Java_script_interface to introduce
 * the features that are shell specific.
 */
class Shell_javascript : public Java_script_interface {
 public:
  using Java_script_interface::Java_script_interface;

  NamingStyle naming_style() const override {
    return NamingStyle::LowerCamelCase;
  };

 private:
  /*
   * load_core_module loads the content of the given module file
   * and inserts the definitions on the JS globals.
   */
  void load_core_module() const;

  void unload_core_module() const;

  void initialize(const std::shared_ptr<IFile_system> &fs = {}) override;

  void finalize() override;

  void init_context_builder() override;

  void output_handler(const char *bytes, size_t length) override;

  void error_handler(const char *bytes, size_t length) override;

  poly_value from_native_object(const Object_bridge_ref &object) const override;

  double call_num_method(poly_value object, const char *method) const;

  shcore::Value native_array(poly_value object);

  shcore::Value native_object(poly_value object);

  shcore::Value native_function(poly_value object);

  shcore::Value to_native_object(poly_value object,
                                 const std::string &class_name) override;

  void load_module(const std::string &path, poly_value module);

 public:
  Value f_source(const Argument_list &args);
  Value f_load_native_module(const Argument_list &args);

  poly_value f_load_module(const std::vector<poly_value> &args);

  Value f_current_module_folder();

  poly_value f_list_native_modules();

  Value f_repr(const Argument_list &args);

  Value f_unrepr(const Argument_list &args);

  poly_value f_type(const std::vector<poly_value> &args);

  std::string get_print_values(const Argument_list &args);

  Value f_print(const Argument_list &args);

  Value f_println(const Argument_list &args);

  bool load_plugin(const std::string &file_name) override;
};
#endif

}  // namespace polyglot
}  // namespace shcore

#endif  // MYSQLSHDK_SCRIPTING_POLYGLOT_SHELL_JAVASCRIPT_H_
