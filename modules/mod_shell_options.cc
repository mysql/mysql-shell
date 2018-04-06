/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/mod_shell_options.h"
#include "modules/mysqlxtest_utils.h"
#include "shellcore/utils_help.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace mysqlsh {

using shcore::Value;

REGISTER_HELP(OPTIONS_BRIEF,
              "Gives access to options impacting shell behavior, accessible "
              "via shell.options.");

std::string &Options::append_descr(std::string &s_out, int indent,
                                   int quote_strings) const {
  shcore::Value::Map_type_ref ops = std::make_shared<shcore::Value::Map_type>();
  for (const auto &name : shell_options->get_named_options())
    (*ops)[name] = Value(shell_options->get(name));
  Value(ops).append_descr(s_out, indent, quote_strings);

  return s_out;
}

bool Options::operator==(const Object_bridge &other) const {
  return class_name() == other.class_name();
}

Value Options::get_member(const std::string &prop) const {
  if (shell_options->has_key(prop)) return shell_options->get(prop);
  return Cpp_object_bridge::get_member(prop);
}

void Options::set_member(const std::string &prop, Value value) {
  if (!shell_options->has_key(prop))
    return Cpp_object_bridge::set_member(prop, value);
  shell_options->set_and_notify(prop, value, false);
}

Options::Options(std::shared_ptr<mysqlsh::Shell_options> options)
    : shell_options(options) {
  for (const auto &opt : options->get_named_options())
    add_property(opt + "|" + opt);

  add_method("set", std::bind(&Options::set, this, std::placeholders::_1));
  add_method("set_persist",
             std::bind(&Options::set_persist, this, std::placeholders::_1));
  add_method("unset", std::bind(&Options::unset, this, std::placeholders::_1));
  add_method("unset_persist",
             std::bind(&Options::unset_persist, this, std::placeholders::_1));
}

REGISTER_HELP(OPTIONS_SET_BRIEF, "Sets value of an option.");
REGISTER_HELP(OPTIONS_SET_PARAM,
              "@param optionName name of the option to set.");
REGISTER_HELP(OPTIONS_SET_PARAM1, "@param value new value for the option.");
/**
 * \ingroup ShellAPI
 * $(OPTIONS_SET_BRIEF)
 *
 * $(OPTIONS_SET_PARAM)
 * $(OPTIONS_SET_PARAM1)
 */
#if DOXYGEN_JS
Undefined Options::set(String optionName, Value value);
#elif DOXYGEN_PY
None Options::set(str optionName, value value);
#endif

shcore::Value Options::set(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("set").c_str());

  try {
    shell_options->set_and_notify(args.string_at(0), args.at(1), false);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("set"));
  return Value();
}

REGISTER_HELP(
    OPTIONS_SET_PERSIST_BRIEF,
    "Sets value of an option and stores it in the configuration file.");
REGISTER_HELP(OPTIONS_SET_PERSIST_PARAM,
              "@param optionName name of the option to set.");
REGISTER_HELP(OPTIONS_SET_PERSIST_PARAM1,
              "@param value new value for the option.");
/**
 * \ingroup ShellAPI
 * $(OPTIONS_SET_PERSIST_BRIEF)
 *
 * $(OPTIONS_SET_PERSIST_PARAM)
 * $(OPTIONS_SET_PERSIST_PARAM1)
 */
#if DOXYGEN_JS
Undefined Options::set_persist(String optionName, Value value);
#elif DOXYGEN_PY
None Options::set_persist(str optionName, value value);
#endif

shcore::Value Options::set_persist(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("set_persist").c_str());

  try {
    shell_options->set_and_notify(args.string_at(0), args.at(1), true);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("set_persist"));
  return Value();
}

REGISTER_HELP(OPTIONS_UNSET_BRIEF, "Resets value of an option to default.");
REGISTER_HELP(OPTIONS_UNSET_PARAM,
              "@param optionName name of the option to reset.");

/**
 * \ingroup ShellAPI
 * $(OPTIONS_UNSET_BRIEF)
 *
 * $(OPTIONS_UNSET_PARAM)
 */
#if DOXYGEN_JS
Undefined Options::unset(String optionName);
#elif DOXYGEN_PY
None Options::unset(str optionName);
#endif

shcore::Value Options::unset(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("unset").c_str());

  try {
    shell_options->unset(args.string_at(0), false);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("unset"));
  return Value();
}

REGISTER_HELP(OPTIONS_UNSET_PERSIST_BRIEF,
              "Resets value of an option to default and removes it from "
              "the configuration file.");
REGISTER_HELP(OPTIONS_UNSET_PERSIST_PARAM,
              "@param optionName name of the option to reset.");

/**
 * \ingroup ShellAPI
 * $(OPTIONS_UNSET_PERSIST_BRIEF)
 *
 * $(OPTIONS_UNSET_PERSIST_PARAM)
 */
#if DOXYGEN_JS
Undefined Options::unset_persist(String optionName);
#elif DOXYGEN_PY
None Options::unset_persist(str optionName);
#endif

shcore::Value Options::unset_persist(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("unset_persist").c_str());

  try {
    shell_options->unset(args.string_at(0), true);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("unset_persist"));
  return Value();
}

}  // namespace mysqlsh
