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

#include "mysqlshdk/scripting/polyglot/shell_javascript.h"

#include <cerrno>
#include <list>
#include <stack>

#include "my_config.h"
#include "mysqlshdk/include/scripting/module_registry.h"
#include "mysqlshdk/include/scripting/obj_date.h"
#include "mysqlshdk/include/scripting/object_factory.h"
#include "mysqlshdk/include/scripting/object_registry.h"
#include "mysqlshdk/include/shellcore/base_shell.h"  // FIXME
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/scripting/polyglot/languages/polyglot_language.h"
#include "mysqlshdk/scripting/polyglot/native_wrappers/polyglot_collectable.h"
#include "mysqlshdk/scripting/polyglot/native_wrappers/polyglot_object_wrapper.h"
#include "mysqlshdk/scripting/polyglot/polyglot_type_conversion.h"
#include "mysqlshdk/scripting/polyglot/polyglot_wrappers/types_polyglot.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_error.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_utils.h"
#include "scripting/jscript_core_definitions.h"

#ifdef HAVE_JS
#include "mysqlshdk/scripting/polyglot/languages/polyglot_javascript.h"
#endif

namespace shcore {
namespace polyglot {

namespace {
struct Source {
  static constexpr const char *name = "source";
  static constexpr std::size_t argc = 1;
  static constexpr auto callback = &Shell_javascript::f_source;
};

struct Load_native_module {
  static constexpr const char *name = "loadNativeModule";
  static constexpr std::size_t argc = 1;
  static constexpr auto callback = &Shell_javascript::f_load_native_module;
};

struct Load_module {
  static constexpr const char *name = "loadModule";
  static constexpr std::size_t argc = 2;
  static constexpr auto callback = &Shell_javascript::f_load_module;
};

struct Current_module_folder {
  static constexpr const char *name = "currentModuleFolder";
  static constexpr auto callback = &Shell_javascript::f_current_module_folder;
};

struct List_native_modules {
  static constexpr const char *name = "listNativeModules";
  static constexpr auto callback = &Shell_javascript::f_list_native_modules;
};

struct Repr {
  static constexpr const char *name = "repr";
  static constexpr std::size_t argc = 1;
  static constexpr auto callback = &Shell_javascript::f_repr;
};

struct Unrepr {
  static constexpr const char *name = "unrepr";
  static constexpr std::size_t argc = 1;
  static constexpr auto callback = &Shell_javascript::f_unrepr;
};

struct Type {
  static constexpr const char *name = "type";
  static constexpr std::size_t argc = 1;
  static constexpr auto callback = &Shell_javascript::f_type;
};

struct Print {
  static constexpr auto callback = &Shell_javascript::f_print;
};

struct Println {
  static constexpr auto callback = &Shell_javascript::f_println;
};

}  // namespace

#ifdef HAVE_JS
/*
 * load_core_module loads the content of the given module file
 * and inserts the definitions on the JS globals.
 */
void Shell_javascript::load_core_module() const {
  shcore::Scoped_naming_style style(naming_style());

  std::string script = "(function (){" + shcore::js_core_module + "})();";

  if (const auto rc = eval("core.js", script, nullptr); rc != poly_ok) {
    throw Polyglot_error(thread(), rc);
  }
}

void Shell_javascript::unload_core_module() const {
  shcore::Scoped_naming_style style(naming_style());

  std::string script = R"((function () {
for (var name in require.__mh.__cache) {
  require.__mh.__cache[name] = null;
  if (name in this) {
    this[name] = null;
  }
}
})();
)";

  if (const auto rc = eval("core.js", script, nullptr); rc != poly_ok) {
    throw Polyglot_error(thread(), rc);
  }
}

void Shell_javascript::initialize(const std::shared_ptr<IFile_system> &fs) {
  Java_script_interface::initialize(fs);

  set_global_function("repr",
                      &native_handler_fixed_args<Shell_javascript, Repr>);
  set_global_function("unrepr",
                      &native_handler_fixed_args<Shell_javascript, Unrepr>);
  set_global_function("type",
                      &polyglot_handler_fixed_args<Shell_javascript, Type>);
  set_global_function("source",
                      &native_handler_fixed_args<Shell_javascript, Source>);
  set_global_function("print",
                      &native_handler_variable_args<Shell_javascript, Print>);
  set_global_function("println",
                      &native_handler_variable_args<Shell_javascript, Println>);
  set_global_function(
      "__list_native_modules",
      &polyglot_handler_no_args<Shell_javascript, List_native_modules>);
  set_global_function(
      "__load_native_module",
      &native_handler_fixed_args<Shell_javascript, Load_native_module>);
  set_global_function(
      "__load_module",
      &polyglot_handler_fixed_args<Shell_javascript, Load_module>);
  set_global_function(
      "__current_module_folder",
      &native_handler_no_args<Shell_javascript, Current_module_folder>);

  load_core_module();
}

void Shell_javascript::finalize() {
  unload_core_module();

  for (const auto global : {
           "__current_module_folder",
           "__load_module",
           "__load_native_module",
           "__list_native_modules",
           "println",
           "print",
           "source",
           "type",
           "unrepr",
           "repr",
       }) {
    set_global(global, nullptr);
  }

  Java_script_interface::finalize();
}

void Shell_javascript::init_context_builder() {
  Java_script_interface::init_context_builder();

  // NOTE: This is required to enable the proper loading of JavaScript
  // Modules
  throw_if_error(poly_context_builder_allow_io, thread(), m_context_builder,
                 true);

  // The current working directory should be specified in the context
  // builder so it is able to track relative paths. When executing a
  // module (i.e. provided through the -f option), the CWD would be the
  // module path so relative paths within the module are properly
  // resolved.
  std::string cwd = shcore::path::getcwd();
  if (auto run_file = mysqlsh::current_shell_options()->get().run_file;
      !run_file.empty()) {
    if (!shcore::path::is_absolute(run_file)) {
      run_file = shcore::path::join_path({cwd, run_file});
    }
    run_file = shcore::path::normalize(run_file);
    cwd = shcore::path::dirname(run_file);
  }

  throw_if_error(poly_context_builder_set_current_working_directory, thread(),
                 m_context_builder, cwd.c_str());
}

void Shell_javascript::output_handler(const char *bytes, size_t length) {
  mysqlsh::current_console()->print(std::string(bytes, length));
}

void Shell_javascript::error_handler(const char *bytes, size_t length) {
  mysqlsh::current_console()->print_diag(std::string(bytes, length));
}

poly_value Shell_javascript::from_native_object(
    const Object_bridge_ref &object) const {
  poly_value result = nullptr;
  if (object && object->class_name() == "Date") {
    std::shared_ptr<Date> date = std::static_pointer_cast<Date>(object);

    // 0 date values can come from MySQL but they're not supported by the JS
    // Date object, so we convert them to null
    if (date->has_date() && date->get_year() == 0 && date->get_month() == 0 &&
        date->get_day() == 0) {
      throw_if_error(poly_create_null, thread(), context(), &result);
    } else if (!date->has_date()) {
      // there's no Time object in JS and we can't use Date to represent time
      // only
      std::string t;
      result = poly_string(date->append_descr(t));
    } else {
      auto source = shcore::str_format(
          "new Date(%d, %d, %d, %d, %d, %d, %d)", date->get_year(),
          date->get_month() - 1, date->get_day(), date->get_hour(),
          date->get_min(), date->get_sec(), date->get_usec() / 1000);

      if (const auto rc = eval("<shell>", source, &result); rc != poly_ok) {
        throw Polyglot_error(thread(), rc);
      }
    }
  }

  return result;
}

double Shell_javascript::call_num_method(poly_value object,
                                         const char *method) const {
  poly_value member;
  double ret_val{0};
  if (get_member(object, method, &member)) {
    bool executable{false};
    throw_if_error(poly_value_can_execute, thread(), member, &executable);

    if (!executable) {
      throw std::runtime_error("Non executable operation!");
    }

    poly_value result;
    throw_if_error(poly_value_execute, thread(), member, nullptr, 0, &result);

    throw_if_error(poly_value_as_double, thread(), result, &ret_val);
  }

  return ret_val;
}

shcore::Value Shell_javascript::native_array(poly_value object) {
  int64_t array_size{0};
  throw_if_error(poly_value_get_array_size, thread(), object, &array_size);

  auto narray = shcore::make_array();
  narray->resize(array_size);

  for (int32_t c = array_size, i = 0; i < c; i++) {
    poly_value item;
    throw_if_error(poly_value_get_array_element, thread(), object, i, &item);

    (*narray)[i] = convert(item);
  }

  return Value(std::move(narray));
}

shcore::Value Shell_javascript::native_object(poly_value object) {
  auto keys = get_member_keys(thread(), context(), object);

  auto dict = shcore::make_dict();
  for (const auto &key : keys) {
    poly_value value;
    throw_if_error(poly_value_get_member, thread(), object, key.c_str(),
                   &value);

    dict->set(key, convert(value));
  }

  return Value(std::move(dict));
}

shcore::Value Shell_javascript::native_function(poly_value object) {
  return Value(std::make_shared<polyglot::Polyglot_function>(shared_from_this(),
                                                             object));
}

shcore::Value Shell_javascript::to_native_object(
    poly_value object, const std::string &class_name) {
  if (class_name == "Date") {
    int year, month, day, hour, min, msec;
    float sec;
    year = static_cast<int>(call_num_method(object, "getFullYear"));
    month = static_cast<int>(call_num_method(object, "getMonth"));
    day = static_cast<int>(call_num_method(object, "getDate"));
    hour = static_cast<int>(call_num_method(object, "getHours"));
    min = static_cast<int>(call_num_method(object, "getMinutes"));
    sec = static_cast<float>(call_num_method(object, "getSeconds"));
    msec = static_cast<int>(call_num_method(object, "getMilliseconds"));
    return Value(std::make_shared<Date>(year, month + 1, day, hour, min, sec,
                                        msec * 1000));
  } else if (class_name == "Array") {
    return native_array(object);
  } else if (class_name == "Object") {
    return native_object(object);
  } else if (class_name == "Function") {
    return native_function(object);
  } else if (class_name == "Error") {
    poly_value poly_cause;
    throw_if_error(poly_value_get_member, thread(), object, "cause",
                   &poly_cause);

    shcore::Value cause = convert(poly_cause);

    if (cause.get_type() != shcore::Value_type::Null &&
        cause.get_type() != shcore::Value_type::Map) {
      poly_value poly_message;
      throw_if_error(poly_value_get_member, thread(), object, "message",
                     &poly_message);
      cause = convert(poly_message);
    }

    return cause;
  }

  return Java_script_interface::to_native_object(object, class_name);
}

void Shell_javascript::load_module(const std::string &path, poly_value module) {
  const auto script_scope = enter_script(path);
  shcore::Scoped_naming_style style(naming_style());

  // load the file
  std::string source;

  if (!load_text_file(path, source)) {
    throw std::runtime_error(
        shcore::str_format("Failed to open '%s'", path.c_str()));
  }

  std::string code =
      "(function(m){(function(exports,module,__filename,__dirname){" + source +
      "})(m.exports,m,m.__filename,m.__dirname)});";

  auto new_context = copy_global_context();

  poly_value executable;
  if (const auto rc =
          poly_context_parse(thread(), new_context, get_language_id(),
                             path.c_str(), code.c_str(), &executable);
      rc != poly_ok) {
    try {
      auto issue = Polyglot_error(thread(), rc);
      throw std::runtime_error(shcore::str_format(
          "Failed to parse '%s': %s", path.c_str(), issue.format().c_str()));
    } catch (const Polyglot_generic_error &) {
      // No additional details...
      throw std::runtime_error(
          shcore::str_format("Failed to parse '%s'", path.c_str()));
    }
  }

  poly_value result;
  throw_if_error(poly_value_execute, thread(), executable, nullptr, 0, &result);

  bool is_function{false};
  if (poly_ok != poly_value_can_execute(thread(), result, &is_function) ||
      !is_function) {
    // this shouldn't happen, as it's our code which creates this function
    throw std::runtime_error(
        shcore::str_format("Failed to execute '%s'.", path.c_str()));
  }

  // // execute the function (and user code), passing the module object
  poly_value args[] = {module};

  poly_value maybe_result;
  throw_if_error(poly_value_execute, thread(), result, args, 1, &maybe_result);

  m_storage->store(new_context);
}

Value Shell_javascript::f_source(const Argument_list &args) {
  Scoped_naming_style style(naming_style());

  const auto str = args[0].as_string();

  // Loads the source content
  std::string source;
  if (load_text_file(str, source)) {
    execute(source, str);
  } else {
    throw std::runtime_error("Error loading script");
  }

  return {};
}

Value Shell_javascript::f_load_native_module(const Argument_list &args) {
  Scoped_naming_style style(naming_style());

  const auto str = args[0].as_string();

  const auto core_modules = Object_factory::package_contents("__modules__");
  if (std::find(core_modules.begin(), core_modules.end(), str) !=
      core_modules.end()) {
    const auto module = Object_factory::call_constructor(
        "__modules__", str, shcore::Argument_list());

    return Value(module);
  }

  return {};
}

poly_value Shell_javascript::f_load_module(
    const std::vector<poly_value> &args) {
  Scoped_naming_style style(naming_style());

  const auto module = to_string(args[0]);

  load_module(module, args[1]);

  return nullptr;
}

Value Shell_javascript::f_current_module_folder() {
  return Value(current_script_folder());
}

poly_value Shell_javascript::f_list_native_modules() {
  const auto core_modules = Object_factory::package_contents("__modules__");

  std::vector<poly_value> module_array;
  module_array.reserve(core_modules.size());

  for (const auto &module : core_modules) {
    module_array.push_back(poly_string(module));
  }

  return poly_array(thread(), context(), module_array);
}

Value Shell_javascript::f_repr(const Argument_list &args) {
  return Value(args[0].repr());
}

Value Shell_javascript::f_unrepr(const Argument_list &args) {
  return Value::parse(args[0].as_string());
}

poly_value Shell_javascript::f_type(const std::vector<poly_value> &args) {
  return type_info(args[0]);
}

std::string Shell_javascript::get_print_values(const Argument_list &args) {
  std::string text;

  for (size_t i = 0; i < args.size(); i++) {
    if (i > 0) text.push_back(' ');

    text += args[i].descr(true);
  }

  return text;
}

Value Shell_javascript::f_print(const Argument_list &args) {
  mysqlsh::current_console()->print(get_print_values(args));

  return {};
}

Value Shell_javascript::f_println(const Argument_list &args) {
  mysqlsh::current_console()->println(get_print_values(args));

  return {};
}

bool Shell_javascript::load_plugin(const std::string &file_name) {
  bool ret_val = true;

  std::string load_code = "require.__mh.__load_module(" +
                          shcore::quote_string(file_name, '"') + ")";

  poly_value executable;
  if (const auto rc =
          poly_context_parse(thread(), context(), get_language_id(),
                             "(load_plugin)", load_code.c_str(), &executable);
      rc != poly_ok) {
    ret_val = false;
    log_error("Error loading JavaScript file '%s':\n\t%s", file_name.c_str(),
              Polyglot_error(thread(), rc).format().c_str());
  } else {
    if (poly_ok !=
        poly_value_execute(thread(), executable, nullptr, 0, nullptr)) {
      ret_val = false;
      log_error("Error loading JavaScript file '%s':\n\tExecution failed: %s",
                file_name.c_str(),
                Polyglot_error(thread(), rc).format().c_str());
    }
  }

  return ret_val;
}
#endif

}  // namespace polyglot
}  // namespace shcore
