#!/usr/bin/env python
#
# Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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
commands to start a replica set, add and remove instance from a
group and display health status of the members of the GR group.
"""

import argparse
import logging
import os
import sys

from mysql_gadgets.common import options, logger
from mysql_gadgets.common.group_replication import (
    OPT_SKIP_CHECK_GR_SCHEMA_COMPLIANCE)
from mysql_gadgets.command.gr_admin import (check, CHECK, join, JOIN, health,
                                            HEALTH, leave, LEAVE, STATUS,
                                            start, START, )
from mysql_gadgets.exceptions import GadgetError

# get script name
_SCRIPT_NAME = os.path.splitext(os.path.split(__file__)[1])[0]

_VALID_COMMANDS = [CHECK, HEALTH, JOIN, LEAVE, STATUS, START]

_AVAILABLE_COMMANDS = ("The available commands are: {0} and {1}"
                       "".format(", ".join(_VALID_COMMANDS[:-1]),
                                 _VALID_COMMANDS[-1]))

_COMMAND_HELP = ("The command to perform. {0}".format(_AVAILABLE_COMMANDS))

_DRY_RUN_HELP = "Run the command without actually making changes."

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

_PEER_SERVER_HELP = ("instance connection information in the form: "
                     "<user>@<host>[:<port>][:<socket>] of another instance "
                     "that already belongs to the replication group.")

_REPLIC_USER_HELP = ("Username that will be used for replication. If it "
                     "does not exist it will be created only if the the "
                     "user given with the server connection information has "
                     "enough privileges. Defaults \"rpl_user\".")

_SERVER_HELP = ("instance connection information in the form: "
                "<user>@<host>[:<port>][:<socket>] of the server "
                "on which the command it will take effect.")

_GROUP_NAME_HELP = ("Group Replication name, must be a valid UUID and will be "
                    "used to represent the Group Replication name, in case "
                    "the UUID is not provided for the 'start' command"
                    "a new UUID will be generated.")

_UPDATE_HELP = ("If given along with --defaults-file option, the check command"
                " will also update the options file provided with "
                "--defaults-file.")

_SKIP_BACKUP_HELP = ("By default when the options file is updated, a backup "
                     "file with the original contents is created. Use this "
                     "option to override the default behavior and avoid "
                     "creating backup files.")

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
    $ {script_name} start --instance=root@localhost:3396

    # Add a new member to the previously created group:
    $ {script_name} join --instance=root@localhost:3397  \\
                          --peer-instance='root@localhost:3396'
""".format(script_name=_SCRIPT_NAME)
)

commands_desc = {
  CHECK: "Checks the configuration in a local options file.",
  HEALTH: "Shows the full status information for the replication group.",
  JOIN: "Checks requirements and adds an instance to the replication group.",
  LEAVE: "Removes an instance from the replication group.",
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
    sub_parser_check.add_argument("--update", "-u", dest="update",
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
                                  def_user_name={"user": "rpl_user"},
                                  help_txt=_REPLIC_USER_HELP, required=False)
    # Add config-file option
    sub_parser_join.add_argument("--defaults-file", dest="option_file",
                                 help=_OPTION_FILE_HELP)
    # add skip backup option
    sub_parser_join.add_argument("--skip-backup", dest="skip_backup",
                                 help=_SKIP_BACKUP_HELP, action="store_true",
                                 required=False)

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

    # Parse provided arguments
    args = _PARSER.parse_args()
    if args.command is None:
        raise _PARSER.error("No command was specified. {0}"
                            "".format(_AVAILABLE_COMMANDS))

    args = _PARSER.parse_args()

    # Setup logging
    logger.setup_logging(program=__name__,
                         verbosity=int(args.verbose),
                         log_filename=args.log_file,
                         output_fmt=args.log_format)

    # Get the provided command
    command = args.command
    # Add replication user password prompt if the --replication-user option
    # was not provided.
    if command == START or command == JOIN:
        options.force_read_password(args, "replication_user",
                                    args.replication_user["user"])

    # ask for passwords
    options.read_passwords(args)

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
        if args.update and args.option_file is None:
            raise _PARSER.error("The use of --update option requires the "
                                "--defaults-file option.")
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

    elif command == HEALTH or command == STATUS:
        # Fill the options
        cmd_options = {"verbose": args.verbose}

    elif command == JOIN:
        cmd_options = {}
        if args.replication_user is not None:
            cmd_options.update({
                "replication_user": args.replication_user["user"],
                "rep_user_passwd": args.replication_user["passwd"],
            })

        # Fill the options
        cmd_options.update({
            "dry_run": args.dry_run,
            "option_file": args.option_file,
            "skip_backup": args.skip_backup,
            "verbose": args.verbose,
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

        # Fill the options
        cmd_options.update({
            "dry_run": args.dry_run,
            "group_name": args.group_name,
            "gr_address": args.gr_address,
            "option_file": args.option_file,
            "skip_backup": args.skip_backup,
            "single_primary": args.single_primary,
            "skip_schema_checks": args.skip_schema_checks,
            "verbose": args.verbose,
        })

    else:
        raise _PARSER.error("The given command '{0}' was not recognized, "
                            "please specify a valid command: {1} and {2}"
                            ".".format(args.command[0],
                                       ", ".join(_VALID_COMMANDS[:-1]),
                                       _VALID_COMMANDS[-1]))

    if cmd_options is not None:
        _LOGGER.debug("Command options: %s", cmd_options)

    # Perform command
    try:
        if command == CHECK:
            check(**cmd_options)

        elif command == HEALTH:
            health(args.server, detailed=True, **cmd_options)

        elif command == STATUS:
            health(args.server, **cmd_options)

        elif command == JOIN:
            if join(args.server, args.peer_server, **cmd_options):
                health(args.server, **cmd_options)

        elif command == LEAVE:
            leave(args.server, **cmd_options)

        elif command == START:
            if start(args.server, **cmd_options):
                health(args.server, **cmd_options)

    except GadgetError as err:
        _LOGGER.error("Error executing the '%s' command: %s", command,
                      str(err))
        sys.exit(1)
    except Exception as err:
        _LOGGER.error("Unexpected error executing the '%s' command: %s",
                      command, str(err))
        sys.exit(1)

    # Operation completed with success.
    _LOGGER.step("Operation completed with success.")
    sys.exit(0)
