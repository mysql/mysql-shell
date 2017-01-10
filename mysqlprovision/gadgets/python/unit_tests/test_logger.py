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
Unit tests for  mysql_gadgets.common.logger module.
"""

import logging
import unittest
import os
import stat
import sys

from mysql_gadgets.common.logger import (disable_logging, setup_logging,
                                         set_log_level,
                                         JSON_FORMAT_TYPE, TEXT_FORMAT_TYPE,
                                         _set_formatter, CustomLevelLogger)

from mysql_gadgets.exceptions import GadgetLogError

# Program name used for tests.
_PROGRAM_NAME = 'test_logger'

# File names used for testing.
_LOG_FILENAME = 'test_logger.txt'
_DEFAULT_LOG_FILENAME = '{0}.log'.format(_PROGRAM_NAME)
_STDOUT_FILENAME = 'test_logger_stdout.txt'
_STDERR_FILENAME = 'test_logger_stderr.txt'


def _check_log_msg(cls, msgtype, msg, logline, terminal=False,
                   output_fmt=TEXT_FORMAT_TYPE):
    """Verify the logged message.

    :param cls:        Test class to execute the test.
    :type cls:         unittest.TestCase subclass
    :param msgtype:    Expected logging message type.
    :param msg:        Expected logging message.
    :param logline:    Logged line to be verified.
    :param terminal:   Indicate if terminal logging is enabled (True) or
                       not (False). Message format for terminal might be
                       different from the one for files.
    :type terminal:    True or False
    :param output_fmt: Output format for the logging messages.
    :type output_fmt: 'TEXT' or 'JSON'
    """
    if output_fmt == TEXT_FORMAT_TYPE:
        if terminal:
            if msgtype in ('CRITICAL', 'ERROR', 'WARNING'):
                unittest.TestCase.assertIn(cls,
                                           "{0}: {1}".format(msgtype, msg),
                                           logline)
            else:
                unittest.TestCase.assertIn(cls, "{0}".format(msg),
                                           logline)
        else:
            unittest.TestCase.assertIn(cls, "[{0}] {1}".format(msgtype, msg),
                                       logline)
    else:
        # Check each JSON element separately (order should not be important).
        unittest.TestCase.assertIn(cls, '"type": "{0}"'.format(msgtype),
                                   logline)
        unittest.TestCase.assertIn(cls, '"msg": "{0}"'.format(msg), logline)
        # Time is not deterministic, just check if JSON element is present.
        unittest.TestCase.assertIn(cls, '"time": "', logline)


def main_logging_test(cls, filename=None, terminal=False,
                      output_fmt=TEXT_FORMAT_TYPE):
    """Main logging test.
    :param cls:        Test class to execute the test
    :type cls:         unittest.TestCase subclass
    :param filename:   Filename (path) of the logging file. If None no logging
                       is performed to a file.
    :type filename:    String
    :param terminal:   Indicate if terminal logging (stdout and stderr) is
                       enabled (True) or not (False).
    :type terminal:    True or False
    :param output_fmt: Output format for the logging messages.
    :type output_fmt: 'TEXT' or 'JSON'
    """
    if terminal:
        # Redirect stdout and stderr to a file.
        old_stdout = sys.stdout
        old_stderr = sys.stderr
        sys.stdout = open(_STDOUT_FILENAME, 'w')
        sys.stderr = open(_STDERR_FILENAME, 'w')
        cls.__class__.redirected = True

    # Set proper log filename considering default name when empty.
    log_filename = filename if filename != '' else _DEFAULT_LOG_FILENAME

    # Setup logging with verbosity > 0
    setup_logging(_PROGRAM_NAME, verbosity=1, quiet=not terminal,
                  log_filename=filename, output_fmt=output_fmt)
    logging.setLoggerClass(CustomLevelLogger)
    log = logging.getLogger(__name__)
    log.debug("debug message")
    log.info("info message")
    log.step("step message")  # pylint: disable=E1101
    log.warning("warning message")
    log.error("error message")
    log.critical("critical message")

    # Reset logging.
    disable_logging()

    if filename is not None:
        # Test logged message to file, all messages logged.
        with open(log_filename, 'r') as f_obj:
            _check_log_msg(cls, 'DEBUG', 'debug message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'INFO', 'info message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'STEP', 'step message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'WARNING', 'warning message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'ERROR', 'error message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'CRITICAL', 'critical message',
                           f_obj.readline(), terminal=False,
                           output_fmt=output_fmt)

    if terminal:
        # Test logged message to terminal , all messages logged.
        with open(_STDOUT_FILENAME, 'r') as f_obj:
            _check_log_msg(cls, 'DEBUG', 'debug message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
            _check_log_msg(cls, 'INFO', 'info message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
            _check_log_msg(cls, 'STEP', 'step message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
            _check_log_msg(cls, 'WARNING', 'warning message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
        with open(_STDERR_FILENAME, 'r') as f_obj:
            _check_log_msg(cls, 'ERROR', 'error message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
            _check_log_msg(cls, 'CRITICAL', 'critical message',
                           f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)

    # Remove all log files.
    if filename is not None:
        os.remove(log_filename)
    if terminal:
        sys.stdout.close()
        sys.stderr.close()
        os.remove(_STDOUT_FILENAME)
        os.remove(_STDERR_FILENAME)
        sys.stdout = open(_STDOUT_FILENAME, 'w')
        sys.stderr = open(_STDERR_FILENAME, 'w')

    # Setup logging with verbosity=0
    setup_logging(_PROGRAM_NAME, verbosity=0, quiet=not terminal,
                  log_filename=filename, output_fmt=output_fmt)
    logging.setLoggerClass(CustomLevelLogger)
    log = logging.getLogger(__name__)
    log.debug("debug message")
    log.info("info message")
    log.step("step message")  # pylint: disable=E1101
    log.warning("warning message")
    log.error("error message")
    log.critical("critical message")
    # Filtered logging message (only to terminal or file).
    log.critical("critical message (terminal only)",
                 extra={'skip_file': True})
    log.critical("critical message (file only)",
                 extra={'skip_term': True})

    # Change log level (only critical message are logged)
    set_log_level(logging.CRITICAL)
    log.debug("debug message (not logged)")
    log.info("info message (not logged)")
    log.step("step message  (not logged)")  # pylint: disable=E1101
    log.warning("warning message (not logged)")
    log.error("error message (not logged)")
    log.critical("critical message (logged)")

    # Reset logging.
    disable_logging()

    if filename is not None:
        # Test logged message to file, no debug message logged.
        with open(log_filename, 'r') as f_obj:
            _check_log_msg(cls, 'INFO', 'info message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'STEP', 'step message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'WARNING', 'warning message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'ERROR', 'error message', f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'CRITICAL', 'critical message',
                           f_obj.readline(),
                           terminal=False, output_fmt=output_fmt)
            _check_log_msg(cls, 'CRITICAL', 'critical message (file only)',
                           f_obj.readline(), terminal=False,
                           output_fmt=output_fmt)
            _check_log_msg(cls, 'CRITICAL', 'critical message (logged)',
                           f_obj.readline(), terminal=False,
                           output_fmt=output_fmt)

    if terminal:
        # Test logged message to terminal, no debug message logged.
        with open(_STDOUT_FILENAME, 'r') as f_obj:
            _check_log_msg(cls, 'INFO', 'info message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
            _check_log_msg(cls, 'STEP', 'step message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
            _check_log_msg(cls, 'WARNING', 'warning message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
        with open(_STDERR_FILENAME, 'r') as f_obj:
            _check_log_msg(cls, 'ERROR', 'error message', f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
            _check_log_msg(cls, 'CRITICAL', 'critical message',
                           f_obj.readline(),
                           terminal=True, output_fmt=output_fmt)
            _check_log_msg(cls, 'CRITICAL', 'critical message (terminal only)',
                           f_obj.readline(), terminal=True,
                           output_fmt=output_fmt)
            _check_log_msg(cls, 'CRITICAL', 'critical message (logged)',
                           f_obj.readline(), terminal=True,
                           output_fmt=output_fmt)

    # Remove all log files.
    if filename is not None:
        os.remove(log_filename)
    if terminal:
        sys.stdout.close()
        sys.stderr.close()
        os.remove(_STDOUT_FILENAME)
        os.remove(_STDERR_FILENAME)

        # Restore stdout and stderr
        sys.stdout = old_stdout
        sys.stderr = old_stderr
        cls.__class__.redirected = False


class TestLogger(unittest.TestCase):
    """TestLogger class.
    """

    @classmethod
    def setUpClass(cls):
        # Keep reference to stdout and stderr to restore at the end.
        # Note: sdtout and stderr will be redirected to a file for testing.
        cls.old_stdout = sys.stdout
        cls.old_stderr = sys.stderr
        cls.redirected = True

    @classmethod
    def tearDownClass(cls):

        # Restore stdout and stderr
        if cls.redirected:
            sys.stdout.close()
            sys.stderr.close()
            sys.stdout = cls.old_stdout
            sys.stderr = cls.old_stderr

        # Remove any remaining log files using for test, making sure everything
        # is properly cleaned even if an error occurs during testing.
        try:
            os.remove(_LOG_FILENAME)
        except OSError:
            # Error if file does not exist.
            # Ok to ignore since it is expected to be removed already.
            pass
        try:
            os.remove(_DEFAULT_LOG_FILENAME)
        except OSError:
            # Error if file does not exist.
            # Ok to ignore since it is expected to be removed already.
            pass
        try:
            os.remove(_STDOUT_FILENAME)
        except OSError:
            # Error if file does not exist.
            # Ok to ignore since it is expected to be removed already.
            pass
        try:
            os.remove(_STDERR_FILENAME)
        except OSError:
            # Error if file does not exist.
            # Ok to ignore since it is expected to be removed already.
            pass

    def test_file_logging(self):
        """Test file logging.
        """
        # Test file logging in TEXT format.
        main_logging_test(self, _LOG_FILENAME, terminal=False,
                          output_fmt=TEXT_FORMAT_TYPE)

        # Test file logging in JSON format.
        main_logging_test(self, _LOG_FILENAME, terminal=False,
                          output_fmt=JSON_FORMAT_TYPE)

    def test_terminal_logging(self):
        """Test terminal logging.
        """
        # Test terminal logging in TEXT format.
        main_logging_test(self, None, terminal=True,
                          output_fmt=TEXT_FORMAT_TYPE)

        # Test terminal logging in JSON format.
        main_logging_test(self, None, terminal=True,
                          output_fmt=JSON_FORMAT_TYPE)

    def test_file_and_terminal_logging(self):
        """Test file and terminal logging.
        """
        # Test file and terminal logging in TEXT format.
        main_logging_test(self, _LOG_FILENAME, terminal=True,
                          output_fmt=TEXT_FORMAT_TYPE)

        # Test file and terminal logging in JSON format.
        main_logging_test(self, _LOG_FILENAME, terminal=True,
                          output_fmt=JSON_FORMAT_TYPE)

    def test_file_logging_default(self):
        """Test default file logging.
        """
        # Test file logging using default filename in TEXT format.
        main_logging_test(self, '', terminal=False,
                          output_fmt=TEXT_FORMAT_TYPE)

        # Test file logging using default filename in JSON format.
        main_logging_test(self, '', terminal=False,
                          output_fmt=JSON_FORMAT_TYPE)

    def test_invalid_logging_format(self):
        """Test invalid logging format error.
        """
        # Test invalid format error.
        self.assertRaises(GadgetLogError, setup_logging, _PROGRAM_NAME,
                          verbosity=1, quiet=False, log_filename=None,
                          output_fmt='INVALID')

        # Test internal use of an invalid handler to set the formatter.
        self.assertRaises(GadgetLogError, _set_formatter, -1,
                          output_fmt=TEXT_FORMAT_TYPE)

    def test_error_with_logfile(self):
        """Test error accessing logging file.
        """
        # Create log file and make it read-only.
        with open(_LOG_FILENAME, 'w') as f_obj:
            f_obj.write("test log file (fake log entry)\n")
        os.chmod(_LOG_FILENAME, stat.S_IREAD)

        # Test error using readonly log file (permission denied).
        self.assertRaises(GadgetLogError, setup_logging, _PROGRAM_NAME,
                          verbosity=1, quiet=False,
                          log_filename=_LOG_FILENAME,
                          output_fmt=TEXT_FORMAT_TYPE)

        # Make file writable again and remove it.
        os.chmod(_LOG_FILENAME, stat.S_IWRITE)
        os.remove(_LOG_FILENAME)


if __name__ == '__main__':
    unittest.main()
