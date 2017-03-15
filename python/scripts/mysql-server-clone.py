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

from __future__ import print_function
import argparse
import logging
import os
import sys

# Check C/Python requirement
from mysql_gadgets import check_connector_python, check_python_version
check_connector_python()

# Check Python version requirement
check_python_version()

from mysql_gadgets.common import options, logger
from mysql_gadgets.command import clone
from mysql_gadgets.adapters import MYSQL_SOURCE, MYSQL_DEST

if __name__ == "__main__":
    # get script name
    script_name = os.path.splitext(os.path.split(__file__)[1])[0]
    # retrieve logger
    logging.setLoggerClass(logger.CustomLevelLogger)
    _LOGGER = logging.getLogger(__name__)

    parser = options.GadgetsArgumentParser(
        prog=script_name,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    # add version
    options.add_version_option(parser)

    # add stdin password
    options.add_stdin_password_option(parser)

    # Add source server
    options.add_store_connection_option(parser, ["--source"], "source",
                                        ask_pass=True)

    # Add destination server
    options.add_store_connection_option(parser, ["--destination"],
                                        "destination", ask_pass=True)

    # Add stream or file option
    file_or_stream = parser.add_mutually_exclusive_group()
    file_or_stream.add_argument(
        "--file", action="store", default=None, const="server_backup.sql",
        dest="file", nargs="?",
        help="If specified {0} does the clone operation "
             "via an intermediate file. First it dumps the contents of the "
             "source server into the image file and then it loads the "
             "contents of the file to the destination server. Cannot be used "
             "together with --stream. If the image file already exists an "
             "error is issued and the clone operation stops. If specified "
             "without any arguments, the default value is 'server_backup.sql'."
             " User must manually remove the image file.".format(parser.prog))

    file_or_stream.add_argument(
        "--stream", action="store_true", dest="stream", default=True,
        help="{0} performs the clone operation without any backup file, "
             "streaming the backup contents directly. This option is used "
             "by default and cannot be used together with --file"
             ".".format(parser.prog))

    # option for the back-end to be used
    parser.add_argument(
        "--using", action="store", default="MySQLDump", dest="using",
        help="Specifies which backup tool {0} will use. By "
             "default it uses MySQLDump.".format(parser.prog))

    # option for log file
    default_log_file = "{0}.log".format(script_name)
    parser.add_argument(
        "--log-file", action="store", const=default_log_file, nargs="?",
        default=None, dest="log_file",
        help="Specifies the log file to be used by the tool. "
             "If specified with no argument it will log to "
             "'{0}'".format(default_log_file))

    # add verbose option
    options.add_verbose_option(parser)

    args = parser.parse_args()

    # setup root logger with received arguments
    try:
        logger.setup_logging(program=parser.prog,
                             verbosity=int(args.verbose),
                             log_filename=args.log_file)
    except Exception as err:
        if args.verbose:
            # in verbose mode show exception traceback.
            _LOGGER.exception(err)
            _LOGGER.error(str(err))
            sys.exit(1)
        else:
            # otherwise print just the error message
            _LOGGER.error(str(err))
            sys.exit(1)

    # read passwords
    options.read_passwords(args)

    # do the cloning
    adapter = args.using
    connection_dict = {MYSQL_SOURCE: args.source,
                       MYSQL_DEST: args.destination}
    try:
        clone.clone_server(connection_dict, adapter, args.file)
    except Exception as err:
        if args.verbose:
            # in verbose mode show exception traceback.
            _LOGGER.exception(err)
            _LOGGER.error(str(err))
            sys.exit(1)
        else:
            # otherwise print just the error message
            _LOGGER.error(str(err))
            sys.exit(1)
    else:
        # operation was successful
        # pylint: disable=E1101
        _LOGGER.step("Operation completed with success.")
        sys.exit(0)
