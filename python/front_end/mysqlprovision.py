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

from mysql_gadgets import check_expected_version

# pylint: disable=wrong-import-position,wrong-import-order
import os
import signal
import sys
import json

from mysql_gadgets.common import logging
from mysql_gadgets.common.group_replication import \
    OPT_SKIP_CHECK_GR_SCHEMA_COMPLIANCE, GR_SSL_DISABLED
from mysql_gadgets.command.gr_admin import check, CHECK, join, health, \
    HEALTH, leave, STATUS, start
from mysql_gadgets.command.sandbox import create_sandbox, stop_sandbox, \
    kill_sandbox, delete_sandbox, start_sandbox, DEFAULT_SANDBOX_DIR, \
    SANDBOX_TIMEOUT, SANDBOX, SANDBOX_CREATE, SANDBOX_DELETE, SANDBOX_KILL, \
    SANDBOX_START, SANDBOX_STOP
from mysql_gadgets.common.constants import PATH_ENV_VAR
from mysql_gadgets.exceptions import GadgetError

# get script name
_SCRIPT_NAME = os.path.splitext(os.path.split(__file__)[1])[0]
# Force script name if file is renamed to __main__.py for zip packaging.
if _SCRIPT_NAME == '__main__':
    _SCRIPT_NAME = 'mysqlprovision'

# Additional supported command names:
JOIN = "join-replicaset"
LEAVE = "leave-replicaset"
START = "start-replicaset"

try:
    import ssl
    HAVE_SSL_SUPPORT = True
except ImportError:
    HAVE_SSL_SUPPORT = False


_VALID_COMMANDS = [CHECK, JOIN, LEAVE, START, SANDBOX]

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

    # Get the provided command
    command = sys.argv[1]

    # Read options from stdin
    data = ""
    while True:
        line = sys.stdin.readline()
        if line == ".\n":
            break
        data += line
    shell_options = json.loads(data)
    cmd_options = shell_options[0]
    del shell_options[0]

    # Handle keyboard interrupt on password retrieve and command execution.
    try:
        # Perform command
        command_error_msg = "executing operation"
        try:
            if command == CHECK:
                if cmd_options.get("update", False):
                    command_error_msg = "configuring instance"
                else:
                    command_error_msg = "checking instance"
                check(**cmd_options)

            elif command == JOIN:
                command_error_msg = "joining instance to cluster"
                server = shell_options[0]
                peer_server = shell_options[1]
                join(server, peer_server, **cmd_options)

            elif command == LEAVE:
                command_error_msg = "leaving cluster"
                server = shell_options[0]
                leave(server, **cmd_options)

            elif command == START:
                command_error_msg = "starting cluster"
                server = shell_options[0]
                start(server, **cmd_options)
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
            import traceback
            traceback.print_exc()
            _LOGGER.error("Unexpected error %s: %s", command_error_msg, str(err))
            sys.exit(1)

        # Operation completed with success.
        sys.exit(0)

    except KeyboardInterrupt:
        _LOGGER.error("keyboard interruption (^C) received, stopping...")
        if os.name == "nt":
            # Using signal.CTRL_C_EVENT on windows will not set any error code.
            # Simulate the Unix signal exit code 130 - Script terminated by Control-C
            # for ngshell to interpreted it as the same way as on linux.
            sys.exit(130)
        else:
            signal.signal(signal.SIGINT, signal.SIG_DFL)
            os.kill(os.getpid(), signal.SIGINT)
