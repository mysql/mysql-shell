#
# Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

import logging
import sys

import mysqlsh

from mysql_gadgets.exceptions import GadgetLogError

shell = mysqlsh.globals.shell

# Custom logging level.
STEP_LOG_LEVEL_NAME = 'STEP'
STEP_LOG_LEVEL_VALUE = 25


def setup_logging(verbosity=0):
    """Setup logging for mysqlprovision.

    This method setup the logging configuration (i.e., verbosity) and logging
    to be performed to the terminal and file through the shell log.

    :param verbosity:    Verbosity level used to setup the logging level.
                         If > 0 then DEBUG otherwise INFO. By default 0.
    :type verbosity:     Integer
    """
    # Set Logger class to use.
    logging.setLoggerClass(CustomLevelLogger)
    # Determine the logging level to use based on verbosity.
    # Note: Set at the root logger level to be used by default.
    logging_level = logging.DEBUG if verbosity > 1 else logging.INFO
    logging.getLogger().setLevel(logging_level)


class CustomLevelLogger(logging.getLoggerClass()):
    """ Custom Logger
    Logs messages directly through shell.
    """

    def __init__(self, name, level=logging.NOTSET):
        """ Constructor.

        :param name:  Name of the logger.
        :type  name:  string
        :param level: Initial logging level for the logger. By default, NOTSET.
        :type level:  string
        """
        # Use old-class initialization for compatibility with Python 2.6
        # and add logging.getLoggerClass() check to avoid recursion issues.
        # super(CustomLevelLogger, self).__init__(name, level)
        if logging.getLoggerClass() != CustomLevelLogger:
            logging.getLoggerClass().__init__(self, name, level)
        else:
            logging.Logger.__init__(self, name, level)
        logging.addLevelName(STEP_LOG_LEVEL_VALUE, STEP_LOG_LEVEL_NAME)

    def step(self, msg, *args, **kwargs):
        """Log message of STEP severity."""
        if kwargs:
            raise GadgetLogError("Invalid use of logging parameters. kwargs "
                                 "not supported for step(): "
                                 "{0}".format(kwargs))
        if self.isEnabledFor(STEP_LOG_LEVEL_VALUE):
            sys.stdout.write(
                u"%s\n" % {"type": "STEP",
                           "msg": (msg % args).encode('utf-8')})
        shell.log("INFO", "mysqlprovision: " + msg % args)

    def debug(self, msg, *args, **kwargs):
        """Log message of DEBUG severity (function overwrite)."""
        if kwargs:
            raise GadgetLogError("Invalid use of logging parameters. kwargs "
                                 "not supported for debug(): "
                                 "{0}".format(kwargs))
        if self.isEnabledFor(logging.DEBUG):
            sys.stdout.write(
                u"%s\n" % {"type": "DEBUG",
                           "msg": (msg % args).encode('utf-8')})
        shell.log("DEBUG", "mysqlprovision: " + msg % args)

    def info(self, msg, *args, **kwargs):
        """Log message of INFO severity (function overwrite)."""
        if kwargs:
            raise GadgetLogError("Invalid use of logging parameters. kwargs "
                                 "not supported for info(): "
                                 "{0}".format(kwargs))
        if self.isEnabledFor(logging.INFO):
            sys.stdout.write(
                u"%s\n" % {"type": "INFO",
                           "msg": (msg % args).encode('utf-8')})
        shell.log("INFO", "mysqlprovision: " + msg % args)

    def warning(self, msg, *args, **kwargs):
        """Log message of WARNING severity (function overwrite)."""
        if kwargs:
            raise GadgetLogError("Invalid use of logging parameters. kwargs "
                                 "not supported for warning(): "
                                 "{0}".format(kwargs))
        if self.isEnabledFor(logging.WARNING):
            sys.stdout.write(
                u"%s\n" % {"type": "WARNING",
                           "msg": (msg % args).encode('utf-8')})
        shell.log("WARNING", "mysqlprovision: " + msg % args)

    def error(self, msg, *args, **kwargs):
        """Log message of ERROR severity (function overwrite)."""
        if kwargs:
            raise GadgetLogError("Invalid use of logging parameters. kwargs "
                                 "not supported for error(): "
                                 "{0}".format(kwargs))
        if self.isEnabledFor(logging.ERROR):
            sys.stderr.write(
                u"%s\n" % {"type": "ERROR",
                           "msg": (msg % args).encode('utf-8')})
        shell.log("ERROR", "mysqlprovision: " + msg % args)

    def critical(self, msg, *args, **kwargs):
        """Log message of CRITICAL severity (function overwrite)."""
        if kwargs:
            raise GadgetLogError("Invalid use of logging parameters. kwargs "
                                 "not supported for critical(): "
                                 "{0}".format(kwargs))
        if self.isEnabledFor(logging.CRITICAL):
            sys.stderr.write(
                u"%s\n" % {"type": "ERROR",
                           "msg": (msg % args).encode('utf-8')})
        shell.log("ERROR", "mysqlprovision: " + msg % args)

    def log(self, level, msg, *args, **kwargs):
        """ Log message of any severity (function overwrite). """
        if kwargs:
            raise GadgetLogError("Invalid use of logging parameters. kwargs "
                                 "not supported for log(): "
                                 "{0}".format(kwargs))
        levelname = logging.getLevelName(level)
        if levelname == "CRITICAL":
            levelname = "ERROR"
        if self.isEnabledFor(level):
            if level >= logging.ERROR:
                sys.stderr.write(
                    u"%s\n" % {"type": levelname,
                               "msg": (msg % args).encode('utf-8')})
            else:
                sys.stdout.write(
                    u"%s\n" % {"type": levelname,
                               "msg": (msg % args).encode('utf-8')})
        shell.log(levelname, "mysqlprovision: " + msg % args)
