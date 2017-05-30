#!/usr/bin/env python
#
# Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

"""
This script is used for manage MySQL Group Replication (GR), providing the
commands to check a server configuration compliance with GR requirements,
start a replica set, add and remove servers from a group, cloning and
displaying the health status of the members of the GR group. It also allows
the creation of MySQL sandbox instances.
"""

from mysql_gadgets import check_connector_python, check_expected_version,\
    check_python_version

# Check Python version requirement
check_python_version()

# Check Connector/Python requirement
check_connector_python()

# pylint: disable=wrong-import-position,wrong-import-order
import argparse
import logging
import os
import sys

from mysql_gadgets.common import options, logger
from mysql_gadgets.common.group_replication import \
    OPT_SKIP_CHECK_GR_SCHEMA_COMPLIANCE, GR_SSL_DISABLED
from mysql_gadgets.command.gr_admin import check, CHECK, join, health, \
    HEALTH, leave, STATUS, start
from mysql_gadgets.command.clone import clone_server
from mysql_gadgets.command.sandbox import create_sandbox, stop_sandbox, \
    kill_sandbox, delete_sandbox, start_sandbox, DEFAULT_SANDBOX_DIR, \
    SANDBOX_TIMEOUT, SANDBOX, SANDBOX_CREATE, SANDBOX_DELETE, SANDBOX_KILL, \
    SANDBOX_START, SANDBOX_STOP
from mysql_gadgets.common.constants import PATH_ENV_VAR
from mysql_gadgets.adapters import MYSQL_DEST, MYSQL_SOURCE
from mysql_gadgets.exceptions import GadgetError

# get script name
_SCRIPT_NAME = os.path.splitext(os.path.split(__file__)[1])[0]
# Force script name if file is renamed to __main__.py for zip packaging.
if _SCRIPT_NAME == '__main__':
    _SCRIPT_NAME = 'mysqlprovision'

# Additional supported command names:
CLONE = "clone"
JOIN = "join-replicaset"
LEAVE = "leave-replicaset"
START = "start-replicaset"

try:
    import ssl
    HAVE_SSL_SUPPORT = True
except ImportError:
    HAVE_SSL_SUPPORT = False


_VALID_COMMANDS = [CLONE, CHECK, HEALTH, JOIN, LEAVE, STATUS, START, SANDBOX]

_AVAILABLE_COMMANDS = ("The available commands are: {0} and {1}"
                       "".format(", ".join(_VALID_COMMANDS[:-1]),
                                 _VALID_COMMANDS[-1]))

_COMMAND_HELP = ("The command to perform. {0}".format(_AVAILABLE_COMMANDS))

_DRY_RUN_HELP = "Run the command without actually making changes."

_IP_WHITELIST_HELP = ("The list of hosts allowed to connect to the instance "
                      "for Group Replication. Specify a custom IP whitelist "
                      "using comma separated list of IP addresses or subnet "
                      "CIDR notation, for example: 192.168.1.0/24,10.0.0.1. "
                      "By default the value is set to AUTOMATIC, allowing "
                      "addresses from the instance private network to be "
                      "automatically set for the whitelist.")

_LOG_FILE_HELP = ("Specifies if the output should be logged to a file and "
                  "the name of this file with no argument it will log to "
                  "'{0}.log'".format(_SCRIPT_NAME))

_LOG_FORMAT_HELP = ("The format of the messages of the logged output. "
                    "Available choices are 'txt' or 'json'. By default 'txt'.")


_OPTION_FILE_HELP = ("The path to the MySQL option file used to start "
                     "the server. This file will be modified with the "
                     "required settings for Group Replication.")

_GR_ADDRESS_HELP = ("The address (host:port) on which the member will expose "
                    "itself to be contacted by the other members of the "
                    "group. Default <host> is the one from instance option "
                    "and default <port> is the number from instance option "
                    "plus 10000.")

_GR_SEEDS_HELP = ("The list of addresses of the group seeds (the servers "
                  "that are part of the group), in the form (host:port)"
                  "[,(host:port)].")

_PEER_SERVER_HELP = ("instance connection information in the form: "
                     "<user>@<host>[:<port>][:<socket>] of another instance "
                     "that already belongs to the replication group.")

_REPLIC_USER_HELP = ("Username that will be used for replication. If it "
                     "does not exist it will be created only if the the "
                     "user given with the server connection information has "
                     "enough privileges. Defaults 'rpl_user@%%'. "
                     "Supported format: <user>[@<host>]. If <host> is not "
                     "specified the user will be created with the wildcard "
                     "'%%' to accept connections from any host.")

_SERVER_HELP = ("instance connection information in the form: "
                "<user>@<host>[:<port>][:<socket>] of the server "
                "on which the command it will take effect.")

_GROUP_NAME_HELP = ("Group Replication name, must be a valid UUID and will be "
                    "used to represent the Group Replication name, in case "
                    "the UUID is not provided for the 'start-replicaset' "
                    "command a new UUID will be generated.")

_UPDATE_HELP = ("Perform the required changes in the server and in the "
                "options file if it was provided with --defaults-file.")

_SKIP_BACKUP_HELP = ("By default when the options file is updated, a backup "
                     "file with the original contents is created. Use this "
                     "option to override the default behavior and avoid "
                     "creating backup files.")

_SKIP_RPL_USER_HELP = "Skip the creation of the replication user."

_SKIP_CHECK_SCHEMA_HELP = ("Skip validation of existing tables for "
                           "compatibility with Group Replication. Tables that"
                           " do not use the InnoDB storage engine or do not "
                           "have a Primary Key cannot receive updates with "
                           "Group Replication.")

_SINGLE_PRIMARY_HELP = ("Sets group replication to single primary mode. In "
                        "this mode only the primary server will take "
                        "incoming updates and all others in the group will "
                        "be secondaries. If the primary fails, the group will"
                        " select a new primary from the available "
                        "secondaries. By Default ON.")

_SAND_PORT_HELP = ("The TCP port on which the MySQL sandbox instance will "
                   "listen for MySQL protocol connections.")
_SAND_XPORT_HELP = ("The TCP port on which the MySQL sandbox instance will "
                    "listen for X protocol connections. Default value is "
                    "<PORT>*10.")
_SAND_BASEDIR_HELP = ("Path to the MySQL installation directory to used as "
                      "basedir for the sandbox.")
_SAND_MYSQLD_HELP = ("Path to mysqld executable. By default searches for "
                     "mysqld on {0}.".format(PATH_ENV_VAR.replace('%', '%%')))
_SAND_MYSQLADMIN_HELP = ("Path to mysqladmin executable. By default searches "
                         "for mysqladmin on {0}."
                         "".format(PATH_ENV_VAR.replace('%', '%%')))
_SAND_MYSQL_SSL_RSA_SETUP_HELP = ("Path to mysql_ssl_rsa_setup executable. By "
                                  "default searches for mysql_ssl_rsa_setup "
                                  "on {0}."
                                  "".format(PATH_ENV_VAR.replace('%', '%%')))
_SAND_SANDBOX_HELP = (
    "Base path for basedir of created MySQL sandbox instances. Default value "
    "is '{0}'.".format(os.path.normpath(os.path.expanduser(
        DEFAULT_SANDBOX_DIR))))
_SAND_SERVER_ID_HELP = ("Server-id value of the MySQL sandbox instance. By "
                        "default it is the same as <PORT>.")
_SAND_TIMEOUT_HELP = ("Timeout in seconds to wait for the sandbox's mysqld "
                      "process to {0}. Default "
                      "value is '{1}' seconds.")
_SAND_OPT_HELP = ("Add option to the [mysqld] section of the option "
                  "file for the MySQL sandbox instance. It can be used to "
                  "override default values. Use option several times to "
                  "specify multiple values. For example: --opt=server_id=1234 "
                  "--opt=log_bin=OFF.")
_SAND_IGNORE_SSL_HELP = ("Create sandbox tries to add SSL support by default "
                         "if not already available. Use this option to allow "
                         "the sandbox instance to be created without SSL "
                         "support.")
_START_HELP = "Start the sandbox instance after its creation."

_EPILOGUE = (
    """Introduction
The {script_name} utility is designed to facilitate the management of MySQL
Group Replication, allowing users to start a replica set (a new replication
group), to add/remove members to/from an existing group. It can also check
if an instance meets all the requirements to successfully create a new
group or to be added to an existing group. This utility is also capable of
modifying MySQL option files of existing instances in order for them to meet
the requirements of Group Replication.

The utility assumes that the given instance has the group replication plugin
installed. The utility will check for Group Replication requirements prior to
configuring and starting the plugin.


Below are some usage examples:

    # Create a new Group Replication group:
    $ {script_name} start-replicaset --instance=root@localhost:3396

    # Add a new member to the previously created group:
    $ {script_name} join-replicaset --instance=root@localhost:3397  \\
                          --peer-instance='root@localhost:3396'
""".format(script_name=_SCRIPT_NAME)
)

commands_desc = {
    CHECK: "Checks the configuration in a local options file.",
    CLONE: "Creates a clone of an existing server.",
    HEALTH: "Shows the full status information for the replication group.",
    JOIN: "Checks requirements and adds an instance to the replication group.",
    LEAVE: "Removes an instance from the replication group.",
    SANDBOX: "Manages a MySQL instance sandbox.",
    SANDBOX_CREATE: "Creates a MySQL instance sandbox for testing purposes.",
    SANDBOX_DELETE: "Deletes the MySQL sandbox folder.",
    SANDBOX_START: "Starts a MySQL instance sandbox.",
    SANDBOX_STOP: "Gracefully stops a MySQL instance sandbox.",
    SANDBOX_KILL: "Forcefully stops a MySQL instance sandbox.",
    START: "Configures a new group replication with the given instance.",
    STATUS: "Shows the status summary for the replication group.",
}

_PARSER = options.GadgetsArgumentParser(
    prog=_SCRIPT_NAME,
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog=_EPILOGUE)

_NOT_REQUIRED_OPTIONS_MSG = ("The following options are not required for "
                             "command '{command}': {not_required}")

_ERROR_OPTION_REQ = "The {0} option is required for the {1} command."

_ERROR_OPTIONS_REQ = ("At least one of {0} options is required for the {1} "
                      "command.")

_ERROR_OPTIONS_SSL_SKIP_ALONG = ("The --ssl-ca, --ssl-ca or --ssl-key option "
                                 "cannot be used together with --ssl-mode="
                                 "{0}.".format(GR_SSL_DISABLED))

_ERROR_OPTIONS_RPL_USER = ("The --replication-user option cannot be used "
                           "together with the --skip-replication-user option.")

_ERROR_INVALID_PORT = ("Port '{port}' is not a valid for {listener} or "
                       "is restricted. Please use a port number >= 1024 and "
                       "<= 65535.")

_ERROR_DUPLICATED_PORT = ("Invalid mysqlx port '{mysqlx_port}', it must be "
                          "different from the server port '{server_port}'.")

_ERROR_NO_SSL_SUPPORT_OPTIONS = (
    "SSL for communicating with MySQL server is currently not supported for "
    "this command. Please execute it again without using any SSL certificate "
    "options for the instance definition.")

_ERROR_NO_SSL_SUPPORT_SESSION = (
    "SSL for communicating with MySQL server is currently not supported for "
    "this command. Please establish a new session to the current instance "
    "without using any SSL certificate options and re-execute the command "
    "again.")

if __name__ == "__main__":
    # retrieve logger
    _LOGGER = logging.getLogger(_SCRIPT_NAME)

    # add version
    options.add_version_option(_PARSER)

    # add command subparsers
    subparsers = _PARSER.add_subparsers(dest="command")

    # Create parent parsers for the different scenarios

    # create common parent parser with common options
    common_parser = options.GadgetsArgumentParser()
    # add verbose to common_parser parser
    options.add_verbose_option(common_parser)
    # Add expected-version option.
    options.add_expected_version_option(common_parser)
    # Add the command argument
    common_parser.add_argument('--log-format', dest='log_format',
                               type=str.upper, default="text",
                               choices=["TEXT", "JSON"], help=_LOG_FORMAT_HELP)
    # option for log file
    common_parser.add_argument("--log-file", action="store",
                               const="{}.log".format(_SCRIPT_NAME), nargs="?",
                               default=None, dest="log_file",
                               help=_LOG_FILE_HELP)

    # For commands that require two servers (--instance, --peer-instance)
    # create two_servers_parser parent parser.
    two_servers_parser = options.GadgetsArgumentParser(add_help=False)
    # Add server option
    options.add_store_connection_option(two_servers_parser,
                                        ["--instance"], "server",
                                        _SERVER_HELP, required=True)
    # Add peer-server option
    options.add_store_connection_option(two_servers_parser,
                                        ["--peer-instance"],
                                        "peer_server", _PEER_SERVER_HELP,
                                        required=True)

    # add option to read passwords from stdin
    options.add_stdin_password_option(two_servers_parser)

    # For commands that require a single server (--instance)
    # create one_server_parser parent parser
    one_server_parser = options.GadgetsArgumentParser(add_help=False)
    # Add source server
    options.add_store_connection_option(one_server_parser,
                                        ["--instance"], "server",
                                        _SERVER_HELP, required=True)

    # add option to read passwords from stdin
    options.add_stdin_password_option(one_server_parser)

    # For commands that the dry-run option create the dry_run_parser
    # parent parser
    dry_run_parser = options.GadgetsArgumentParser(add_help=False)
    # add dry-run option
    dry_run_parser.add_argument("--dry-run", dest="dry_run",
                                help=_DRY_RUN_HELP, action="store_true",
                                required=False)

    # Create subparsers for commands
    # create parser for clone command
    sub_parser_clone = subparsers.add_parser(
        CLONE, help=commands_desc[CLONE], description=commands_desc[CLONE],
        parents=[common_parser, two_servers_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)

    # create parser for check command
    sub_parser_check = subparsers.add_parser(
        CHECK, help=commands_desc[CHECK], description=commands_desc[CHECK],
        parents=[common_parser, dry_run_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)
    # add options for the check command
    options.add_store_connection_option(sub_parser_check,
                                        ["--instance"], "server",
                                        _SERVER_HELP, required=False)
    # Add config-file option
    sub_parser_check.add_argument("--defaults-file", dest="option_file",
                                  help=_OPTION_FILE_HELP)
    # add update option
    sub_parser_check.add_argument("--update", dest="update",
                                  help=_UPDATE_HELP, action="store_true",
                                  required=False)
    # add skip backup option
    sub_parser_check.add_argument("--skip-backup", dest="skip_backup",
                                  help=_SKIP_BACKUP_HELP, action="store_true",
                                  required=False)
    # add option to read passwords from stdin
    options.add_stdin_password_option(sub_parser_check)

    # create parser for health command
    sub_parser_health = subparsers.add_parser(
        HEALTH, help=commands_desc[HEALTH], description=commands_desc[HEALTH],
        parents=[common_parser, one_server_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)

    # create parser for join command
    sub_parser_join = subparsers.add_parser(
        JOIN, help=commands_desc[JOIN], description=commands_desc[JOIN],
        parents=[common_parser, two_servers_parser, dry_run_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)
    # Add replication-user option
    options.add_store_user_option(sub_parser_join, ["--replication-user"],
                                  dest="replication_user",
                                  def_user_name=None,
                                  help_txt=_REPLIC_USER_HELP, required=False)
    # Add config-file option
    sub_parser_join.add_argument("--defaults-file", dest="option_file",
                                 help=_OPTION_FILE_HELP)
    # add gr-host option
    sub_parser_join.add_argument("--gr-bind-address", dest="gr_address",
                                  help=_GR_ADDRESS_HELP)
    sub_parser_join.add_argument("--group-seeds", dest="group_seeds",
                                  help=_GR_SEEDS_HELP)
    # Add the SSL options
    options.add_ssl_options(sub_parser_join)

    # add skip backup option
    sub_parser_join.add_argument("--skip-backup", dest="skip_backup",
                                 help=_SKIP_BACKUP_HELP, action="store_true",
                                 required=False)
    # add skip rpl_user option
    sub_parser_join.add_argument("--skip-replication-user",
                                 dest="skip_rpl_user",
                                 help=_SKIP_RPL_USER_HELP, action="store_true",
                                 required=False)

    # add ip-whitelist option
    sub_parser_join.add_argument("--ip-whitelist", dest="ip_whitelist",
                                 help=_IP_WHITELIST_HELP, required=False)

    # create parser for leave command
    sub_parser_leave = subparsers.add_parser(
        LEAVE, help=commands_desc[LEAVE],
        parents=[common_parser, one_server_parser, dry_run_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)
    # Add config-file option
    sub_parser_leave.add_argument("--defaults-file", dest="option_file",
                                  help=_OPTION_FILE_HELP)
    # add skip backup option
    sub_parser_leave.add_argument("--skip-backup", dest="skip_backup",
                                  help=_SKIP_BACKUP_HELP, action="store_true",
                                  required=False)

    # create parser for status command
    subparsers.add_parser(STATUS, help=commands_desc[STATUS],
                          description=commands_desc[STATUS],
                          parents=[common_parser, one_server_parser],
                          formatter_class=argparse.RawDescriptionHelpFormatter,
                          add_help=False)

    # create parser for start command
    sub_parser_start = subparsers.add_parser(
        START, help=commands_desc[START], description=commands_desc[START],
        parents=[common_parser, one_server_parser, dry_run_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)
    # add gr-host option
    sub_parser_start.add_argument("--gr-bind-address", dest="gr_address",
                                  help=_GR_ADDRESS_HELP)
    sub_parser_start.add_argument("--group-seeds", dest="group_seeds",
                                  help=_GR_SEEDS_HELP)
    # Add the SSL options
    options.add_ssl_options(sub_parser_start)

    # Add replication-user option
    options.add_store_user_option(sub_parser_start, ["--replication-user"],
                                  dest="replication_user",
                                  def_user_name={"user": "rpl_user"},
                                  help_txt=_REPLIC_USER_HELP, required=False)
    # Add group-name option
    options.add_uuid_option(sub_parser_start, "--group-name",
                            dest="group_name", help_txt=_GROUP_NAME_HELP)
    # Add config-file option
    sub_parser_start.add_argument("--defaults-file", dest="option_file",
                                  help=_OPTION_FILE_HELP)
    # add skip backup option
    sub_parser_start.add_argument("--skip-backup", dest="skip_backup",
                                  help=_SKIP_BACKUP_HELP, action="store_true",
                                  required=False)
    # Add single-primary option
    sub_parser_start.add_argument('--single-primary', dest='single_primary',
                                  type=str.upper, default="ON",
                                  choices=["ON", "OFF"],
                                  help=_SINGLE_PRIMARY_HELP)

    # Add option to skip GR validation of schemas
    # (note: only valid for bootstrap)
    sub_parser_start.add_argument(OPT_SKIP_CHECK_GR_SCHEMA_COMPLIANCE,
                                  dest="skip_schema_checks",
                                  action="store_true", required=False,
                                  help=_SKIP_CHECK_SCHEMA_HELP)

    # add ip-whitelist option
    sub_parser_start.add_argument("--ip-whitelist", dest="ip_whitelist",
                                  help=_IP_WHITELIST_HELP, required=False)

    # create parser for sandbox command
    sub_parser_sandbox = subparsers.add_parser(
        SANDBOX, help=commands_desc[SANDBOX],
        description=commands_desc[SANDBOX],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=True)

    # Create subparsers for sandbox command
    subparsers_sandbox = sub_parser_sandbox.add_subparsers(dest="sandbox_cmd")
    # Create subparser
    sub_parser_sandbox_create = subparsers_sandbox.add_parser(
        SANDBOX_CREATE, help=commands_desc[SANDBOX_CREATE],
        description=commands_desc[SANDBOX_CREATE],
        parents=[common_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)
    # Start subparser
    sub_parser_sandbox_start = subparsers_sandbox.add_parser(
        SANDBOX_START, help=commands_desc[SANDBOX_START],
        description=commands_desc[SANDBOX_START],
        parents=[common_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)
    # Stop subparser
    sub_parser_sandbox_stop = subparsers_sandbox.add_parser(
        SANDBOX_STOP, help=commands_desc[SANDBOX_STOP],
        description=commands_desc[SANDBOX_STOP],
        parents=[common_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)
    # kill subparser
    sub_parser_sandbox_kill = subparsers_sandbox.add_parser(
        SANDBOX_KILL, help=commands_desc[SANDBOX_KILL],
        description=commands_desc[SANDBOX_KILL],
        parents=[common_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)
    # delete subparser
    sub_parser_sandbox_delete = subparsers_sandbox.add_parser(
        SANDBOX_DELETE, help=commands_desc[SANDBOX_DELETE],
        description=commands_desc[SANDBOX_DELETE],
        parents=[common_parser],
        formatter_class=argparse.RawDescriptionHelpFormatter, add_help=False)

    # add port option to the subparsers that will use it
    options.add_port_option(sub_parser_sandbox_create, _SAND_PORT_HELP)
    options.add_port_option(sub_parser_sandbox_start, _SAND_PORT_HELP)
    options.add_port_option(sub_parser_sandbox_stop, _SAND_PORT_HELP)
    options.add_port_option(sub_parser_sandbox_kill, _SAND_PORT_HELP)
    options.add_port_option(sub_parser_sandbox_delete, _SAND_PORT_HELP)
    # add x-port option
    sub_parser_sandbox_create.add_argument("--mysqlx-port", dest="mysqlx_port",
                                           help=_SAND_XPORT_HELP, type=int,
                                           required=False, default=None)
    # add basedir option
    sub_parser_sandbox_create.add_argument("--basedir", dest="basedir",
                                           help=_SAND_BASEDIR_HELP,
                                           required=False, default=None)
    # add mysqld option
    sub_parser_sandbox_create.add_argument("--mysqld", dest="mysqld_path",
                                           help=_SAND_MYSQLD_HELP,
                                           required=False, default=None)
    sub_parser_sandbox_start.add_argument("--mysqld", dest="mysqld_path",
                                          help=_SAND_MYSQLD_HELP,
                                          required=False, default=None)
    # add mysqladmin option
    sub_parser_sandbox_create.add_argument("--mysqladmin",
                                           dest="mysqladmin_path",
                                           help=_SAND_MYSQLADMIN_HELP,
                                           required=False, default=None)
    # add mysql_ssl_rsa_setup option
    sub_parser_sandbox_create.add_argument("--mysql_ssl_rsa_setup",
                                           dest="mysql_ssl_rsa_setup_path",
                                           help=_SAND_MYSQL_SSL_RSA_SETUP_HELP,
                                           required=False, default=None)

    # add sandboxdir option
    sub_parser_sandbox_create.add_argument("--sandboxdir",
                                           dest="sandbox_base_dir",
                                           help=_SAND_SANDBOX_HELP,
                                           required=False, default=None)
    sub_parser_sandbox_start.add_argument("--sandboxdir",
                                          dest="sandbox_base_dir",
                                          help=_SAND_SANDBOX_HELP,
                                          required=False, default=None)
    sub_parser_sandbox_stop.add_argument("--sandboxdir",
                                         dest="sandbox_base_dir",
                                         help=_SAND_SANDBOX_HELP,
                                         required=False, default=None)
    sub_parser_sandbox_kill.add_argument("--sandboxdir",
                                         dest="sandbox_base_dir",
                                         help=_SAND_SANDBOX_HELP,
                                         required=False, default=None)
    sub_parser_sandbox_delete.add_argument("--sandboxdir",
                                           dest="sandbox_base_dir",
                                           help=_SAND_SANDBOX_HELP,
                                           required=False, default=None)
    # add server-id option
    sub_parser_sandbox_create.add_argument("--server-id", dest="server_id",
                                           help=_SAND_SERVER_ID_HELP,
                                           required=False, default=None,
                                           type=int)

    # Add timeout option
    _timout_help_txt = _SAND_TIMEOUT_HELP.format(
        "start listening for connections", SANDBOX_TIMEOUT)
    sub_parser_sandbox_create.add_argument("--timeout", dest="timeout",
                                           help=_timout_help_txt,
                                           required=False, default=None,
                                           type=int)
    sub_parser_sandbox_start.add_argument("--timeout", dest="timeout",
                                          help=_timout_help_txt,
                                          required=False, default=None,
                                          type=int)
    _timout_help_txt = _SAND_TIMEOUT_HELP.format(
        "stop", SANDBOX_TIMEOUT)
    sub_parser_sandbox_stop.add_argument("--timeout", dest="timeout",
                                         help=_timout_help_txt,
                                         required=False, default=None,
                                         type=int)

    # Add opt option
    sub_parser_sandbox_create.add_argument("--opt", dest="opt",
                                           help=_SAND_OPT_HELP,
                                           required=False, action="append")
    sub_parser_sandbox_start.add_argument("--opt", dest="opt",
                                          help=_SAND_OPT_HELP,
                                          required=False, action="append")

    # Add skip ssl option
    sub_parser_sandbox_create.add_argument("--ignore-ssl-error",
                                           dest="ignore_ssl_error",
                                           action="store_true",
                                           help=_SAND_IGNORE_SSL_HELP)

    # add option to read passwords from stdin
    options.add_stdin_password_option(sub_parser_sandbox_create)
    options.add_stdin_password_option(sub_parser_sandbox_stop)

    # add start option
    sub_parser_sandbox_create.add_argument("--start", dest="start",
                                           action="store_true",
                                           help=_START_HELP)

    # Parse provided arguments
    args = _PARSER.parse_args()
    if args.command is None:
        raise _PARSER.error("No command was specified. {0}"
                            "".format(_AVAILABLE_COMMANDS))

    # Setup logging
    logger.setup_logging(program=__name__,
                         verbosity=int(args.verbose),
                         log_filename=args.log_file,
                         output_fmt=args.log_format)

    # Check if the expected version is compatible with the current one.
    if hasattr(args, "expected_version") and args.expected_version:
        try:
            check_expected_version(args.expected_version)
        except GadgetError:
            _, err, _ = sys.exc_info()
            _LOGGER.error(str(err))
            sys.exit(1)
    else:
        _LOGGER.error("%s is an internal tool of MySQL InnoDB cluster. "
                      "Please use the MySQL Shell instead.", _SCRIPT_NAME)
        sys.exit(1)

    # Get the provided command
    command = args.command
    # Add replication user password prompt if the --replication-user option
    # was not provided.

    if command == START:
        options.force_read_password(args, "replication_user",
                                    args.replication_user["user"])
    if command == JOIN:
        if not args.skip_rpl_user and args.replication_user is None:
            # set default value for replication_user
            args.replication_user = {"user": "rpl_user"}
            options.force_read_password(args, "replication_user",
                                        args.replication_user["user"])
    # ask for passwords
    options.read_passwords(args)

    # SSL connection information
    if hasattr(args, "server") and isinstance(args.server, dict):
        ssl_server_conn_dict = {}
        if args.server_ssl_ca:
            ssl_server_conn_dict["ssl_ca"] = args.server_ssl_ca
        if args.server_ssl_cert:
            ssl_server_conn_dict["ssl_cert"] = args.server_ssl_cert
        if args.server_ssl_key:
            ssl_server_conn_dict["ssl_key"] = args.server_ssl_key
        if ssl_server_conn_dict:
            ssl_server_conn_dict["ssl"] = True
            # throw an error if Python has no ssl support but we are trying to
            # use ssl to encrypt the communications
            if not HAVE_SSL_SUPPORT:
                if command == JOIN:
                    # on the join replicaset command the server argument is
                    # the server we want to add to the group. This means any
                    # ssl connection information does not come from the current
                    # session but from instance definition parameter of
                    # <cluster>.addInstance
                    _LOGGER.error(_ERROR_NO_SSL_SUPPORT_OPTIONS)
                    sys.exit(1)
                else:
                    # on the rest of the commands the server ssl information
                    # provided to mysqlprovision comes from the connection
                    # being used in the shell session.
                    _LOGGER.error(_ERROR_NO_SSL_SUPPORT_SESSION)
                    sys.exit(1)

        args.server.update(ssl_server_conn_dict)

    if hasattr(args, "peer_server") and isinstance(args.peer_server, dict):
        ssl_peer_conn_dict = {}
        if args.peer_server_ssl_ca:
            ssl_peer_conn_dict["ssl_ca"] = args.peer_server_ssl_ca
        if args.peer_server_ssl_cert:
            ssl_peer_conn_dict["ssl_cert"] = args.peer_server_ssl_cert
        if args.peer_server_ssl_key:
            ssl_peer_conn_dict["ssl_key"] = args.peer_server_ssl_key
        if ssl_peer_conn_dict:
            ssl_peer_conn_dict["ssl"] = True
            if not HAVE_SSL_SUPPORT:
                # on the join command (only command that receives a peer
                # server) the peer_server ssl information provided to
                # mysqlprovision comes from the connection being used in the
                # shell session.
                _LOGGER.error(_ERROR_NO_SSL_SUPPORT_SESSION)
                sys.exit(1)
        args.peer_server.update(ssl_peer_conn_dict)

    if command == SANDBOX:
        sandbox_pw = None
        # Check that port number is valid
        if args.port < 1024 or args.port > 65535:
            raise _PARSER.error(_ERROR_INVALID_PORT.format(port=args.port,
                                                           listener="server"))

        # Check that mysqlx-port number is valid
        if hasattr(args, "mysqlx_port") and args.mysqlx_port is not None and \
           (args.mysqlx_port < 1024 or args.mysqlx_port > 65535):
            raise _PARSER.error(_ERROR_INVALID_PORT.format(
                port=args.mysqlx_port, listener="mysqlx"))

        # The mysqlx-port must be different from the server port.
        if hasattr(args, "mysqlx_port") and args.mysqlx_port is not None and \
           args.mysqlx_port == args.port:
            raise _PARSER.error(_ERROR_DUPLICATED_PORT.format(
                mysqlx_port=args.mysqlx_port, server_port=args.port))

        if args.sandbox_cmd == SANDBOX_CREATE:
            # Read extra password for root user of the sandbox
            sandbox_pw = options.read_extra_password(
                "Enter a password to be set for the root user of the MySQL "
                "sandbox (root@localhost): ", read_from_stdin=args.stdin_pw)
        if args.sandbox_cmd == SANDBOX_STOP:
            # Read extra password for root user of the sandbox
            sandbox_pw = options.read_extra_password(
                "Enter the password for the root user of the MySQL "
                "sandbox (root@localhost): ", read_from_stdin=args.stdin_pw)

    _LOGGER.debug("Setting options for command: %s", command)
    cmd_options = None
    # Verify specified options for the given command.
    if command == CHECK:
        # Check only requires the args.server (server conn information)
        # or option_file
        if args.option_file is None and args.server is None:
            raise _PARSER.error(_ERROR_OPTIONS_REQ.format("--defaults-file "
                                                          "or --instance",
                                                          command))

        if args.skip_backup and not args.update:
            raise _PARSER.error("The use of --skip-backup option requires the "
                                "--update option.")
        cmd_options = {
            "server": args.server,
            "update": args.update,
            "skip_backup": args.skip_backup,
            "option_file": args.option_file,
            "verbose": args.verbose,
        }

    elif command == CLONE:
        adapter_name = "MySQLDump"
        connection_dict = {MYSQL_SOURCE: args.peer_server,
                           MYSQL_DEST: args.server}

    elif command == HEALTH or command == STATUS:
        # Fill the options
        cmd_options = {"verbose": args.verbose}

    elif command == JOIN:
        cmd_options = {}

        if (args.ssl_ca or args.ssl_cert or args.ssl_key) and \
                args.ssl_mode == GR_SSL_DISABLED:
            raise _PARSER.error(_ERROR_OPTIONS_SSL_SKIP_ALONG)

        if args.replication_user and args.skip_rpl_user:
            raise _PARSER.error(_ERROR_OPTIONS_RPL_USER)

        if args.replication_user is not None:
            cmd_options.update({
                "replication_user": args.replication_user["user"],
                "rep_user_passwd": args.replication_user["passwd"],
            })

        # Fill the options
        cmd_options.update({
            "dry_run": args.dry_run,
            "option_file": args.option_file,
            "ip_whitelist": args.ip_whitelist,
            "gr_address": args.gr_address,
            "group_seeds": args.group_seeds,
            "skip_backup": args.skip_backup,
            "ssl_mode": args.ssl_mode,
            "ssl_ca": args.ssl_ca,
            "ssl_cert": args.ssl_cert,
            "ssl_key": args.ssl_key,
            "verbose": args.verbose,
            "skip_rpl_user": args.skip_rpl_user,
        })

    elif command == LEAVE:
        # Fill the options
        cmd_options = {
            "dry_run": args.dry_run,
            "option_file": args.option_file,
            "skip_backup": args.skip_backup,
            "verbose": args.verbose,
        }

    elif command == START:
        cmd_options = {}
        if args.replication_user is not None:
            cmd_options.update({
                "replication_user": args.replication_user["user"],
                "rep_user_passwd": args.replication_user["passwd"],
            })

        if (args.ssl_ca or args.ssl_cert or args.ssl_key) and \
                args.ssl_mode == GR_SSL_DISABLED:
            raise _PARSER.error(_ERROR_OPTIONS_SSL_SKIP_ALONG)

        # Fill the options
        cmd_options.update({
            "dry_run": args.dry_run,
            "group_name": args.group_name,
            "gr_address": args.gr_address,
            "group_seeds": args.group_seeds,
            "ip_whitelist": args.ip_whitelist,
            "option_file": args.option_file,
            "skip_backup": args.skip_backup,
            "single_primary": args.single_primary,
            "skip_schema_checks": args.skip_schema_checks,
            "ssl_mode": args.ssl_mode,
            "ssl_ca": args.ssl_ca,
            "ssl_cert": args.ssl_cert,
            "ssl_key": args.ssl_key,
            "verbose": args.verbose,
        })

    elif command == SANDBOX:
        # get dictionary from the namespace object
        cmd_options = vars(args)
        # remove None values to use method defaults
        for key, val in list(cmd_options.items()):
            if val is None:
                del cmd_options[key]
        # set root password for create and stop cmd.
        cmd_options["passwd"] = sandbox_pw
    else:
        raise _PARSER.error("The given command '{0}' was not recognized, "
                            "please specify a valid command: {1} and {2}"
                            ".".format(args.command[0],
                                       ", ".join(_VALID_COMMANDS[:-1]),
                                       _VALID_COMMANDS[-1]))

    if cmd_options is not None:
        # Hide the password
        hide_pw_dict = {"passwd": "******"}
        cmd_options_hidden_pw = cmd_options.copy()
        cmd_options_hidden_pw.update(hide_pw_dict)
        if "rep_user_passwd" in cmd_options_hidden_pw:
            cmd_options_hidden_pw["rep_user_passwd"] = "******"

        # \ are converted to \\ internally (in dictionaries), and \\ will be
        # printed instead of \ if we try to print the full dictionary.
        # Therefore, the dictionary needs to be converted to string and the
        # \\ converted to \, in order to be handled correctly by the logger.
        dic_msg = str(cmd_options_hidden_pw)
        dic_msg = dic_msg.replace("\\\\", "\\")
        _LOGGER.debug("Command options: %s", dic_msg)

    # Perform command
    command_error_msg = "executing operation"
    try:
        if command == CHECK:
            command_error_msg = "checking instance"
            check(**cmd_options)

        elif command == CLONE:
            command_error_msg = "cloning instance"
            clone_server(connection_dict, adapter_name)

        elif command == HEALTH:
            command_error_msg = "checking cluster health"
            health(args.server, detailed=True, **cmd_options)

        elif command == STATUS:
            command_error_msg = "checking cluster status"
            health(args.server, **cmd_options)

        elif command == JOIN:
            command_error_msg = "joining instance to cluster"
            if join(args.server, args.peer_server, **cmd_options):
                command_error_msg = "checking cluster health"
                health(args.server, **cmd_options)

        elif command == LEAVE:
            command_error_msg = "leaving cluster"
            leave(args.server, **cmd_options)

        elif command == START:
            command_error_msg = "starting cluster"
            if start(args.server, **cmd_options):
                command_error_msg = "checking cluster health"
                health(args.server, **cmd_options)
        elif command == SANDBOX:
            sandbox_cmd = cmd_options["sandbox_cmd"]
            command = '{0} {1}'.format(command, sandbox_cmd)
            command_error_msg = "executing sandbox operation"
            if sandbox_cmd == SANDBOX_START:
                command_error_msg = "starting sandbox"
                start_sandbox(**cmd_options)
            elif sandbox_cmd == SANDBOX_CREATE:
                command_error_msg = "creating sandbox"
                create_sandbox(**cmd_options)
            elif sandbox_cmd == SANDBOX_STOP:
                command_error_msg = "stopping sandbox"
                stop_sandbox(**cmd_options)
            elif sandbox_cmd == SANDBOX_KILL:
                command_error_msg = "killing sandbox"
                kill_sandbox(**cmd_options)
            elif sandbox_cmd == SANDBOX_DELETE:
                command_error_msg = "deleting sandbox"
                delete_sandbox(**cmd_options)

    except GadgetError:
        _, err, _ = sys.exc_info()
        _LOGGER.error("Error %s: %s", command_error_msg, str(err))
        sys.exit(1)
    except Exception:  # pylint: disable=broad-except
        _, err, _ = sys.exc_info()
        _LOGGER.error("Unexpected error %s: %s", command_error_msg, str(err))
        sys.exit(1)

    # Operation completed with success.
    sys.exit(0)
