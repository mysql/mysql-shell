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

#include "mysqlshdk/scripting/polyglot/polyglot_context.h"

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
#include "mysqlshdk/scripting/polyglot/shell_javascript.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_error.h"
#include "mysqlshdk/scripting/polyglot/utils/polyglot_utils.h"
#include "scripting/jscript_core_definitions.h"

#ifdef HAVE_JS
#include "mysqlshdk/scripting/polyglot/languages/polyglot_javascript.h"
#endif

namespace shcore {
namespace polyglot {

Polyglot_context::Polyglot_context(Object_registry *registry, Language type) {
  m_common_context.initialize({});
  m_language = get_language(type);
  assert(m_language);
  m_language->initialize();
  set_global("globals", Value(registry->_registry));
}

std::shared_ptr<Polyglot_language> Polyglot_context::get_language(
    Language type) {
#ifdef HAVE_JS
  if (type == Language::JAVASCRIPT) {
    return std::make_shared<Shell_javascript>(&m_common_context);
  }
#endif
  return {};
}

poly_context Polyglot_context::context() const { return m_language->context(); }

poly_thread Polyglot_context::thread() const { return m_language->thread(); }

Polyglot_context::~Polyglot_context() {
  set_global("globals", Value{});
  m_language->finalize();
  m_common_context.finalize();
}

void Polyglot_context::set_global(const std::string &name, const Value &value) {
  m_language->set_global(name, convert(value));
}

Value Polyglot_context::get_global(const std::string &name) {
  return convert(m_language->get_global(name).get());
}

std::tuple<polyglot::Store, std::string> Polyglot_context::get_global_polyglot(
    const std::string &name) {
  auto global{m_language->get_global(name)};

  if (global.get() == nullptr) {
    return {};
  }

  if (m_language->is_object(global.get())) {
    auto obj_type = m_language->to_string(type_info(global.get()));

    if (shcore::str_beginswith(obj_type, "m.")) {
      obj_type = obj_type.substr(2);
    }

    return std::make_tuple(std::move(global), std::move(obj_type));
  } else if (is_native_type(thread(), global.get(), Collectable_type::OBJECT)) {
    auto obj_type = m_language->to_string(type_info(global.get()));
    if (shcore::str_beginswith(obj_type, "m.")) {
      obj_type = obj_type.substr(2);
    }

    return std::make_tuple(std::move(global), std::move(obj_type));
  } else {
    return {};
  }
}

std::vector<std::pair<bool, std::string>> Polyglot_context::list_globals() {
  auto globals = m_language->globals();
  auto names = globals->get_members();

  std::vector<std::pair<bool, std::string>> ret;
  for (auto &name : names) {
    auto member = get_member(m_language->thread(), globals->get(), name);
    ret.emplace_back(is_executable(m_language->thread(), member),
                     std::move(name));
  }

  // In graal, the print function for being built in is not returned as member
  // of the globals object, so it is injected here
  ret.emplace_back(true, "print");

  return ret;
}

const std::vector<std::string> &Polyglot_context::keywords() const {
  return m_language->keywords();
}

std::tuple<bool, polyglot::Store, std::string> Polyglot_context::get_member_of(
    const poly_value obj, const std::string &name) {
  auto member = get_member(thread(), obj, name);
  if (member) {
    auto type = to_string(thread(), type_info(member));
    if (shcore::str_beginswith(type, "m.")) {
      type = type.substr(2);
    }

    return {is_executable(thread(), member), Store(thread(), member),
            std::move(type)};
  }

  return {false, Store(nullptr), ""};
}

std::vector<std::pair<bool, std::string>> Polyglot_context::get_members_of(
    const poly_value obj) {
  auto props = get_member_keys(thread(), context(), obj);
  std::vector<std::pair<bool, std::string>> keys;

  for (auto &prop : props) {
    auto member = get_member(thread(), obj, prop);
    keys.emplace_back(is_executable(thread(), member), std::move(prop));
  }

  return keys;
}

Value Polyglot_context::convert(poly_value value) const {
  return m_language->convert(value);
}

poly_value Polyglot_context::convert(const Value &value) const {
  return m_language->convert(value);
}

poly_value Polyglot_context::type_info(poly_value value) const {
  return m_language->type_info(value);
}

void Polyglot_context::set_argv(const std::vector<std::string> &argv) {
  shcore::Value args_value = get_global("sys").as_object()->get_member("argv");
  auto args = args_value.as_array();
  args->clear();
  for (auto &arg : argv) {
    args->push_back(Value(arg));
  }
}

std::pair<Value, bool> Polyglot_context::execute(const std::string &code_str,
                                                 const std::string &source) {
  shcore::Scoped_naming_style style(m_language->naming_style());
  try {
    return m_language->execute(code_str, source);
  } catch (Polyglot_error &error) {
    if (m_language->is_terminating() &&
        (error.is_interrupted() ||
         shcore::str_beginswith(error.message(), "Interrupted"))) {
      mysqlsh::current_console()->print_diag(
          "Script execution interrupted by user.");
    } else {
      throw Exception::scripting_error(error.format(false));
    }
  }

  return {Value(), true};
}

std::pair<Value, bool> Polyglot_context::execute_interactive(
    const std::string &code_str, Input_state *r_state, bool flush) noexcept {
  shcore::Scoped_naming_style style(m_language->naming_style());

  m_language->clear_is_terminating();

  *r_state = Input_state::Ok;

  // automatically release unreferenced local variables
  Polyglot_scope scope{thread()};

  poly_value result;
  auto rc = m_language->eval(k_origin_shell, code_str.c_str(), &result);

  if (rc == poly_ok) {
    return {convert(result), false};
  }

  auto exception = Polyglot_error(thread(), rc);

  const auto console = mysqlsh::current_console();

  if (m_language->is_terminating() &&
      (exception.is_interrupted() ||
       shcore::str_beginswith(exception.message(), "Interrupted"))) {
    console->print_diag("Script execution interrupted by user.");
  } else {
    // If there's no more input then the interpreter should not
    // continue in continued mode so any error should bubble up
    if (!flush) {
      // check if this was an error of type
      // - SyntaxError: "Expected } but found eof"
      // - SyntaxError: "Missing close quote"
      // which we treat as a multiline mode trigger or
      // - SyntaxError: Expected an operand but found error
      // which may be a sign of unfinished C style comment
      constexpr std::string_view k_unexpected_eof = "but found eof";
      constexpr std::string_view k_expected_binding =
          "expected BindingIdentifier or BindingPattern";
      constexpr std::string_view k_missing_closing_quote =
          "Missing close quote";
      constexpr std::string_view k_expected_operand =
          "Expected an operand but found error";

      if (exception.is_syntax_error()) {
        auto message = exception.message();
        if (message.find(k_unexpected_eof) != std::string::npos ||
            message.find(k_expected_binding) != std::string::npos ||
            // For backticks the column is reported as 0 all the time,
            // wihle for the double and single quote it is always the
            // quote location + 1
            (message == k_missing_closing_quote &&
             exception.column().has_value() && *exception.column() == 0)) {
          *r_state = Input_state::ContinuedBlock;
        } else if (message == k_expected_operand) {
          auto comment_pos = code_str.rfind("/*");

          while (comment_pos != std::string::npos &&
                 code_str.find("*/", comment_pos + 2) == std::string::npos) {
            rc = poly_context_parse(
                thread(), context(), m_language->get_language_id(),
                k_origin_shell, code_str.substr(0, comment_pos).c_str(),
                &result);

            if (poly_ok == rc) {
              *r_state = Input_state::ContinuedSingle;
            } else {
              exception = Polyglot_error(thread(), rc);
              message = exception.message();
              if (message.find(k_unexpected_eof) != std::string::npos ||
                  message.find(k_expected_binding) != std::string::npos) {
                *r_state = Input_state::ContinuedBlock;
              } else if (message == k_expected_operand) {
                comment_pos = code_str.rfind("/*", comment_pos - 1);
                continue;
              }
            }
            break;
          }
        }
      }
    }

    if (*r_state == Input_state::Ok) {
      console->print_diag(exception.format(true));
    }
  }

  return {Value(), true};
}

void Polyglot_context::terminate() { m_language->terminate(); }

bool Polyglot_context::load_plugin(const Plugin_definition &plugin) {
  return m_language->load_plugin(plugin.file);
}

}  // namespace polyglot
}  // namespace shcore
