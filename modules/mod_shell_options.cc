/*
 * Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

#include "modules/mod_shell_options.h"
#include "modules/mysqlxtest_utils.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "shellcore/utils_help.h"
#include "utils/utils_file.h"
#include "utils/utils_general.h"
#include "utils/utils_string.h"

namespace mysqlsh {

using shcore::Value;

REGISTER_HELP_OBJECT(options, shell);
REGISTER_HELP_TOPIC_WITH_BRIEF_TEXT(OPTIONS, R"*(
Gives access to options impacting shell behavior.

The options object acts as a dictionary, it may contain the following
attributes:

@li <b>autocomplete.nameCache</b>: true if auto-refresh of DB object name cache
is enabled. The <b>\\rehash</b> command can be used for manual refresh.

@li <b>batchContinueOnError</b>: read-only, boolean value to indicate if the
execution of an SQL script in batch mode shall continue if errors occur

@li <b>connectTimeout</b>: float, default connection timeout used by Shell
sessions, in seconds

@li <b>credentialStore.excludeFilters</b>: array of URLs for which automatic
password storage is disabled, supports glob characters '*' and '?'

@li <b>credentialStore.helper</b>: name of the credential helper to use to
fetch/store passwords; a special value "default" is supported to use platform
default helper; a special value "<disabled>" is supported to disable the
credential store

@li <b>credentialStore.savePasswords</b>: controls automatic password storage,
allowed values: "always", "prompt" or "never"

@li <b>dba.connectTimeout</b>: float, default connection timeout used for
sessions created in AdminAPI operations, in seconds

@li <b>dba.connectivityChecks</b>: bool, checks SSL settings and network
connectivity between instances when creating a cluster, replicaset or
clusterset, or adding an instance to one

@li <b>dba.gtidWaitTimeout</b>: timeout value in seconds to wait for GTIDs to be
synchronized

@li <b>dba.logSql</b>: 0..2, log SQL statements executed by AdminAPI operations:
0 - logging disabled; 1 - log statements other than SELECT and SHOW; 2 - log
all statements. Option takes precedence over <b>--log-sql</b> in Dba.* context,
if enabled.

@li <b>dba.restartWaitTimeout</b>: timeout in seconds to wait for MySQL server
to come back after a restart during clone recovery

@li <b>dba.versionCompatibilityChecks</b>: bool, checks version compatibility
for asynchronous replication when managing a ReplicaSet, ClusterSet, or a
Cluster with Read-Replicas.

@li <b>defaultCompress</b>: Enable compression in client/server protocol by
default in global shell sessions.

@li <b>defaultMode</b>: shell mode to use when shell is started, allowed values:
"js", "py", "sql" or "none"

@li <b>devapi.dbObjectHandles</b>: true to enable schema collection and table
name aliases in the db object, for DevAPI operations

@li <b>history.autoSave</b>: true to save command history when exiting the shell

@li <b>history.maxSize</b>: number of entries to keep in command history

@li <b>history.sql.ignorePattern</b>: colon separated list of glob patterns to
filter out of the command history in SQL mode

@li <b>history.sql.syslog</b>: true to log filtered interactive commands to the
system log, filtering of commands depends on the value of
<b>history.sql.ignorePattern</b>

@li <b>interactive</b>: read-only, boolean value that indicates if the shell is
running in interactive mode

@li <b>logFile</b>: read-only, path to the log file. Use <b>--log-file</b> to
change the location.

@li <b>logLevel</b>: current log level, allowed values are integers between 1
and 8, or one of: "none", "internal", "error", "warning", "info", "debug",
"debug2", "debug3". If value is prefixed with '@', log messages are also written
to the stderr.

@li <b>logSql</b>: Log SQL statements: off - none of SQL statements will be
logged; error (default) - SQL statement with error message will be logged only
when error occurs; on - all SQL statements will be logged except these which
match any of <b>logSql.ignorePattern</b> and <b>logSql.ignorePatternUnsafe</b>
glob pattern; all - all SQL statements will be logged except these which match
any of <b>logSql.ignorePatternUnsafe</b> glob pattern; unfiltered - all SQL
statements will be logged

@li <b>logSql.ignorePattern</b>: Colon separated list of glob patterns to filter
out SQL queries to be logged when <b>logSql</b> is set to "on". Default:
*SELECT*:*SHOW*

@li <b>logSql.ignorePatternUnsafe</b>: Colon separated list of glob patterns to
filter out SQL queries to be logged when <b>logSql</b> is set to "all". Default:
*IDENTIFIED*:*PASSWORD*

@li <b>mysqlPluginDir</b>: Directory for client-side authentication plugins

@li <b>oci.configFile</b>: Path to OCI (Oracle Cloud Infrastructure)
configuration file

@li <b>oci.profile</b>: Specify which section in oci.configFile will be used as
profile settings

@li <b>outputFormat</b>: Deprecated, use <b>resultFormat</b> instead

@li <b>pager</b>: string which specifies the external command which is going to
be used to display the paged output

@li <b>passwordsFromStdin</b>: boolean value that indicates if the shell should
read passwords from stdin instead of the tty

@li <b>resultFormat</b>: controls the type of output produced for SQL results

@li <b>sandboxDir</b>: default path where the new sandbox instances for InnoDB
cluster will be deployed

@li <b>showColumnTypeInfo</b>: display column type information in SQL mode.
Please be aware that output may depend on the protocol you are using to
connect to the server, e.g. DbType field is approximated when using X protocol.

@li <b>showWarnings</b>: boolean value to indicate whether warnings shall be
included when printing a SQL result

@li <b>ssh.bufferSize</b> integer, default 10240 bytes, used for tunnel data
transfer

@li <b>ssh.configFile</b> string, default empty, custom path for SSH
configuration. If not defined, the standard SSH paths will be used
(<b>~/.ssh/config</b>).

@li <b>useWizards</b>: read-only, boolean value to indicate if interactive
prompting and wizards are enabled by default in AdminAPI and others. Use
<b>--no-wizard</b> to disable.

@li <b>verbose</b>: 0..4, verbose output level. If >0, additional output that
may help diagnose issues is printed to the screen. Larger values mean more
verbose. Default is 0.

The <b>resultFormat</b> option supports the following values to modify the
format of printed query results:

@li <b>table</b>: tabular format with a ascii character frame (default)
@li <b>tabbed</b>: tabular format with no frame, columns separated by tabs
@li <b>vertical</b>: displays the outputs vertically, one line per column value
@li <b>json</b>: same as json/pretty
@li <b>ndjson</b>: newline delimited JSON, same as json/raw
@li <b>json/array</b>: one JSON document per line, inside an array
@li <b>json/pretty</b>: pretty printed JSON
@li <b>json/raw</b>: one JSON document per line
)*");

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

  expose("set", &Options::set, "option_name", "value")->cli(false);
  expose("setPersist", &Options::set_persist, "option_name", "value")->cli();
  expose("unset", &Options::unset, "option_name")->cli(false);
  expose("unsetPersist", &Options::unset_persist, "option_name")->cli();
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
#else
void Options::set(const std::string &option_name, const Value &value) {
  shell_options->set_and_notify(option_name, value, false);
}
#endif

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
#else
void Options::set_persist(const std::string &option_name, const Value &value) {
  shell_options->set_and_notify(option_name, value, true);
}
#endif

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

void Options::unset(const std::string &option_name) {
  shell_options->unset(option_name, false);
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

void Options::unset_persist(const std::string &option_name) {
  shell_options->unset(option_name, true);
}

}  // namespace mysqlsh
