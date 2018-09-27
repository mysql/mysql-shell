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

REGISTER_HELP_OBJECT(options, shell);
REGISTER_HELP(OPTIONS_BRIEF,
              "Gives access to options impacting shell behavior.");

REGISTER_HELP(OPTIONS_DETAIL,
              "The options object acts as a dictionary, it may contain "
              "the following attributes:");
REGISTER_HELP(OPTIONS_DETAIL1,
              "@li autocomplete.nameCache: true if auto-refresh of DB object "
              "name cache is "
              "enabled. The \\rehash command can be used for manual refresh");
REGISTER_HELP(OPTIONS_DETAIL2,
              "@li batchContinueOnError: read-only, "
              "boolean value to indicate if the "
              "execution of an SQL script in batch "
              "mode shall continue if errors occur");
REGISTER_HELP(OPTIONS_DETAIL3,
              "@li credentialStore.excludeFilters: array of URLs for which "
              "automatic password storage is disabled, supports glob "
              "characters '*' and '?'");
REGISTER_HELP(OPTIONS_DETAIL4,
              "@li credentialStore.helper: name of the credential helper to "
              "use to fetch/store passwords; a special value \"default\" is "
              "supported to use platform default helper; a special value "
              "\"@<disabled>\" is supported to disable the credential store");
REGISTER_HELP(OPTIONS_DETAIL5,
              "@li credentialStore.savePasswords: controls automatic password "
              "storage, allowed values: \"always\", \"prompt\" or \"never\" ");
REGISTER_HELP(OPTIONS_DETAIL6,
              "@li dba.gtidWaitTimeout: timeout value in seconds to wait for "
              "GTIDs to be synchronized");
REGISTER_HELP(OPTIONS_DETAIL7,
              "@li defaultMode: shell mode to use when shell is started, "
              "allowed values: \"js\", \"py\", \"sql\" or \"none\" ");
REGISTER_HELP(OPTIONS_DETAIL8,
              "@li devapi.dbObjectHandles: true to enable schema collection "
              "and table name aliases in the db "
              "object, for DevAPI operations.");
REGISTER_HELP(OPTIONS_DETAIL9,
              "@li history.autoSave: true "
              "to save command history when exiting the shell");
REGISTER_HELP(OPTIONS_DETAIL10,
              "@li history.maxSize: number "
              "of entries to keep in command history");
REGISTER_HELP(OPTIONS_DETAIL11,
              "@li history.sql.ignorePattern: colon separated list of glob "
              "patterns to filter"
              " out of the command history in SQL mode");
REGISTER_HELP(OPTIONS_DETAIL12,
              "@li interactive: read-only, boolean "
              "value that indicates if the shell is "
              "running in interactive mode");
REGISTER_HELP(OPTIONS_DETAIL13, "@li logLevel: current log level");
REGISTER_HELP(OPTIONS_DETAIL14,
              "@li outputFormat: controls the type of "
              "output produced for SQL results.");
REGISTER_HELP(OPTIONS_DETAIL15,
              "@li pager: string which specifies the external command which is "
              "going to be used to display the paged output");
REGISTER_HELP(OPTIONS_DETAIL16,
              "@li passwordsFromStdin: boolean value that indicates if the "
              "shell should read passwords from stdin instead of the tty");
REGISTER_HELP(OPTIONS_DETAIL17,
              "@li sandboxDir: default path where the "
              "new sandbox instances for InnoDB "
              "cluster will be deployed");
REGISTER_HELP(OPTIONS_DETAIL18,
              "@li showWarnings: boolean value to "
              "indicate whether warnings shall be "
              "included when printing an SQL result");
REGISTER_HELP(OPTIONS_DETAIL19,
              "@li useWizards: read-only, boolean value "
              "to indicate if the Shell is using the "
              "interactive wrappers (wizard mode)");

REGISTER_HELP(OPTIONS_DETAIL20,
              "The outputFormat option supports the following values:");
REGISTER_HELP(OPTIONS_DETAIL21,
              "@li table: displays the output in table format (default)");
REGISTER_HELP(OPTIONS_DETAIL22, "@li json: displays the output in JSON format");
REGISTER_HELP(
    OPTIONS_DETAIL23,
    "@li json/raw: displays the output in a JSON format but in a single line");
REGISTER_HELP(
    OPTIONS_DETAIL24,
    "@li vertical: displays the outputs vertically, one line per column value");

std::string &Options::append_descr(std::string &s_out, int indent,
                                   int quote_strings) const {
  shcore::Value::Map_type_ref ops = std::make_shared<shcore::Value::Map_type>();
  for (const auto &name : shell_options->get_named_options())
    (*ops)[name] = Value(shell_options->get(name));
  Value(ops).append_descr(s_out, indent, quote_strings);

  return s_out;
}

void Options::append_json(shcore::JSON_dumper &dumper) const {
  dumper.start_object();
  for (const auto &name : shell_options->get_named_options()) {
    dumper.append_value(name, Value(shell_options->get(name)));
  }
  dumper.end_object();
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
  add_method("setPersist",
             std::bind(&Options::set_persist, this, std::placeholders::_1));
  add_method("unset", std::bind(&Options::unset, this, std::placeholders::_1));
  add_method("unsetPersist",
             std::bind(&Options::unset_persist, this, std::placeholders::_1));
}

REGISTER_HELP_FUNCTION(set, options);
REGISTER_HELP(OPTIONS_SET_BRIEF, "Sets value of an option.");
REGISTER_HELP(OPTIONS_SET_PARAM,
              "@param optionName name of the option to set.");
REGISTER_HELP(OPTIONS_SET_PARAM1, "@param value new value for the option.");
/**
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

REGISTER_HELP_FUNCTION(setPersist, options);
REGISTER_HELP(
    OPTIONS_SETPERSIST_BRIEF,
    "Sets value of an option and stores it in the configuration file.");
REGISTER_HELP(OPTIONS_SETPERSIST_PARAM,
              "@param optionName name of the option to set.");
REGISTER_HELP(OPTIONS_SETPERSIST_PARAM1,
              "@param value new value for the option.");
/**
 * $(OPTIONS_SETPERSIST_BRIEF)
 *
 * $(OPTIONS_SETPERSIST_PARAM)
 * $(OPTIONS_SETPERSIST_PARAM1)
 */
#if DOXYGEN_JS
Undefined Options::setPersist(String optionName, Value value);
#elif DOXYGEN_PY
None Options::set_persist(str optionName, value value);
#endif

shcore::Value Options::set_persist(const shcore::Argument_list &args) {
  args.ensure_count(2, get_function_name("setPersist").c_str());

  try {
    shell_options->set_and_notify(args.string_at(0), args.at(1), true);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("setPersist"));
  return Value();
}

REGISTER_HELP_FUNCTION(unset, options);
REGISTER_HELP(OPTIONS_UNSET_BRIEF, "Resets value of an option to default.");
REGISTER_HELP(OPTIONS_UNSET_PARAM,
              "@param optionName name of the option to reset.");

/**
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

REGISTER_HELP_FUNCTION(unsetPersist, options);
REGISTER_HELP(OPTIONS_UNSETPERSIST_BRIEF,
              "Resets value of an option to default and removes it from "
              "the configuration file.");
REGISTER_HELP(OPTIONS_UNSETPERSIST_PARAM,
              "@param optionName name of the option to reset.");

/**
 * $(OPTIONS_UNSETPERSIST_BRIEF)
 *
 * $(OPTIONS_UNSETPERSIST_PARAM)
 */
#if DOXYGEN_JS
Undefined Options::unsetPersist(String optionName);
#elif DOXYGEN_PY
None Options::unset_persist(str optionName);
#endif

shcore::Value Options::unset_persist(const shcore::Argument_list &args) {
  args.ensure_count(1, get_function_name("unsetPersist").c_str());

  try {
    shell_options->unset(args.string_at(0), true);
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION(get_function_name("unsetPersist"));
  return Value();
}

}  // namespace mysqlsh
