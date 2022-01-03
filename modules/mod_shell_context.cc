/*
 * Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

#include "modules/mod_shell_context.h"
#include <utility>
#include "modules/mod_shell.h"
#include "mysqlshdk/include/scripting/types.h"

namespace mysqlsh {

Delegate_wrapper::Delegate_wrapper(
    const shcore::Option_pack_ref<Shell_context_wrapper_options> &callbacks)
    : Interpreter_delegate(this, &Delegate_wrapper::deleg_print,
                           &Delegate_wrapper::deleg_prompt,
                           &Delegate_wrapper::deleg_print_error,
                           &Delegate_wrapper::deleg_print_diag) {
  m_deleg_print_func = callbacks->deleg_print_func();
  m_deleg_error_func = callbacks->deleg_error_func();
  m_deleg_diag_func = callbacks->deleg_diag_func();
  m_deleg_prompt_func = callbacks->deleg_prompt_func();
}

shcore::Value Delegate_wrapper::call_delegate(
    void *cdata, const char *text,
    shcore::Function_base_ref Delegate_wrapper::*func) {
  Delegate_wrapper *self = reinterpret_cast<Delegate_wrapper *>(cdata);
  if (self && text && self->*func) {
    shcore::Argument_list r;
    r.push_back(shcore::Value(text));
    return (self->*func)->invoke(r);
  }
  return shcore::Value();
}

shcore::Value Delegate_wrapper::call_prompt_delegate(
    void *cdata, const char *text,
    const shcore::prompt::Prompt_options &options,
    shcore::Function_base_ref Delegate_wrapper::*func) {
  using shcore::prompt::Prompt_type;
  Delegate_wrapper *self = reinterpret_cast<Delegate_wrapper *>(cdata);
  if (self && text && self->*func) {
    shcore::Argument_list r;
    r.push_back(shcore::Value(text));

    auto options_dict = shcore::make_dict();

    (*options_dict)[shcore::prompt::k_type] =
        shcore::Value(shcore::prompt::to_string(options.type));

    if (!options.title.empty()) {
      (*options_dict)[shcore::prompt::k_title] = shcore::Value(options.title);
    }

    if (!options.description.empty()) {
      auto desc_option = shcore::make_array();
      for (const auto &paragraph : options.description) {
        desc_option->emplace_back(paragraph);
      }

      (*options_dict)[shcore::prompt::k_description] =
          shcore::Value(desc_option);
    }

    if (options.default_value) {
      (*options_dict)[shcore::prompt::k_default_value] =
          shcore::Value(options.default_value);
    }

    if (options.type == Prompt_type::CONFIRM) {
      (*options_dict)[shcore::prompt::k_yes_label] =
          shcore::Value(options.yes_label);
      (*options_dict)[shcore::prompt::k_no_label] =
          shcore::Value(options.no_label);
      if (!options.alt_label.empty()) {
        (*options_dict)[shcore::prompt::k_alt_label] =
            shcore::Value(options.alt_label);
      }
    } else if (options.type == Prompt_type::SELECT) {
      auto item_list = shcore::make_array();
      for (const auto &item : options.select_items) {
        item_list->emplace_back(item);
      }
      (*options_dict)[shcore::prompt::k_options] = shcore::Value(item_list);
    }

    r.push_back(shcore::Value(options_dict));

    return (self->*func)->invoke(r);
  }

  return shcore::Value();
}

shcore::Prompt_result Delegate_wrapper::get_prompt_value(
    const shcore::Value &prompt_value, std::string *ret) {
  if (prompt_value.type == shcore::Array) {
    auto array = prompt_value.as_array();
    if (!(*array)[0].as_bool()) {
      return shcore::Prompt_result::Cancel;
    }
    *ret = (*array)[1].as_string();
    return shcore::Prompt_result::Ok;
  } else {
    throw shcore::Exception::runtime_error("Invalid return value.");
  }
  return shcore::Prompt_result::Cancel;
}

shcore::Prompt_result Delegate_wrapper::deleg_prompt(
    void *cdata, const char *text,
    const shcore::prompt::Prompt_options &options, std::string *ret) {
  return get_prompt_value(
      call_prompt_delegate(cdata, text, options,
                           &Delegate_wrapper::m_deleg_prompt_func),
      ret);
}

bool Delegate_wrapper::deleg_print(void *cdata, const char *text) {
  call_delegate(cdata, text, &Delegate_wrapper::m_deleg_print_func);
  return true;
}

bool Delegate_wrapper::deleg_print_error(void *cdata, const char *text) {
  call_delegate(cdata, text, &Delegate_wrapper::m_deleg_error_func);
  return true;
}

bool Delegate_wrapper::deleg_print_diag(void *cdata, const char *text) {
  call_delegate(cdata, text, &Delegate_wrapper::m_deleg_diag_func);
  return true;
}

void Shell_context_wrapper_options::on_start_unpack(
    const shcore::Dictionary_t &options) {
  m_options = options;
}

const shcore::Option_pack_def<Shell_context_wrapper_options>
    &Shell_context_wrapper_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Shell_context_wrapper_options>()
          .on_start(&Shell_context_wrapper_options::on_start_unpack)
          .optional("logFile", &Shell_context_wrapper_options::m_log_file)
          .optional("printDelegate",
                    &Shell_context_wrapper_options::m_deleg_print_func)
          .optional("errorDelegate",
                    &Shell_context_wrapper_options::m_deleg_error_func)
          .optional("diagDelegate",
                    &Shell_context_wrapper_options::m_deleg_diag_func)
          .optional("promptDelegate",
                    &Shell_context_wrapper_options::m_deleg_prompt_func);
  return opts;
}

REGISTER_HELP_CLASS_MODE(ShellContextWrapper, shell, shcore::Help_mode::PYTHON);
REGISTER_HELP(SHELLCONTEXTWRAPPER_BRIEF,
              "Holds shell context which is used by a thread, gives access to "
              "the shell context object through <<<getShell>>>().");

Shell_context_wrapper::Shell_context_wrapper(
    const shcore::Option_pack_ref<Shell_context_wrapper_options> &callbacks,
    const std::shared_ptr<Shell_options> &opts) {
  if (!mysqlshdk::utils::in_main_thread()) {
    m_logger = std::make_shared<Scoped_logger>(shcore::Logger::create_instance(
        callbacks->log_file().c_str(), opts.get()->get().log_to_stderr,
        opts.get()->get().log_level));
    m_log_sql = std::make_shared<Scoped_log_sql>(
        std::make_shared<shcore::Log_sql>(*(opts.get())));
    m_interrupt =
        std::make_shared<Scoped_interrupt>(shcore::Interrupts::create(nullptr));
  }

  m_delegate_wrapper = std::make_unique<Delegate_wrapper>(callbacks);
  m_mysql_shell = std::make_shared<Mysql_shell>(opts, m_delegate_wrapper.get());
  m_mysql_shell->finish_init();
  add_property("globals");
  expose("getShell", &Shell_context_wrapper::get_shell);
  expose("finalize", &Shell_context_wrapper::finalize);
}

shcore::Value Shell_context_wrapper::get_member(const std::string &prop) const {
  if (m_finalized) {
    throw std::logic_error("The Shell context has been finalized already.");
  }

  if (prop == "globals") {
    return shcore::Value(get_globals());
  } else {
    return Cpp_object_bridge::get_member(prop);
  }
}

REGISTER_HELP_FUNCTION_MODE(getShell, ShellContextWrapper, PYTHON);
REGISTER_HELP_FUNCTION_TEXT(SHELLCONTEXTWRAPPER_GETSHELL, R"*(
  Shell object in its own context

@returns Shell

  This function returns shell object that has its own scope.
)*");
#if DOXYGEN_PY
/**
 * $(SHELLCONTEXTWRAPPER_GETSHELL_BRIEF)
 *
 * $(SHELLCONTEXTWRAPPER_GETSHELL)
 */
Shell ShellContextWrapper::get_shell();
#endif
std::shared_ptr<Shell> Shell_context_wrapper::get_shell() const {
  if (m_finalized) {
    throw std::logic_error("The Shell context has been finalized already.");
  }

  return m_mysql_shell->get_shell();
}

REGISTER_HELP_PROPERTY_MODE(globals, ShellContextWrapper, PYTHON);
REGISTER_HELP(SHELLCONTEXTWRAPPER_GLOBALS_BRIEF,
              "Gives access to the global objects registered on the context.");
#if DOXYGEN_PY
/**
 * $(SHELLCONTEXTWRAPPER_GLOBALS_BRIEF)
 */
dict ShellContextWrapper::globals;
#endif
shcore::Dictionary_t Shell_context_wrapper::get_globals() const {
  return m_mysql_shell->get_shell()->get_globals(
      shcore::IShell_core::Mode::Python);
}

REGISTER_HELP_FUNCTION_MODE(finalize, ShellContextWrapper, PYTHON);
REGISTER_HELP_FUNCTION_TEXT(SHELLCONTEXTWRAPPER_FINALIZE, R"*(
  Explicitly finalizes the Shell Context object releasing the log file handle.
)*");
void Shell_context_wrapper::finalize() {
  m_logger.reset();
  m_finalized = true;
}
}  // namespace mysqlsh
