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
Module to setup and manage logging feature for MySQL Orchestrator Gadgets.
"""

import collections
import logging
import sys

from mysql_gadgets.exceptions import GadgetLogError

PY2 = int(sys.version[0]) == 2

# Format of the log messages.
DEFAULT_DATE_FORMAT = '%Y-%m-%d %H:%M:%S %p'
DEFAULT_FILE_MSG_FORMAT = "(%(asctime)s) [%(levelname)s] %(message)s"
DEFAULT_TERM_MSG_FORMAT = "%(message)s"
JSON_MSG_FORMAT = ('{"type": "%(levelname)s", "msg": "%(message)s", '
                   '"time": "%(asctime)s"}')

# Define the supported types for the output format.
TEXT_FORMAT_TYPE = 'TEXT'
JSON_FORMAT_TYPE = 'JSON'
OUTPUT_FORMAT_TYPES = (TEXT_FORMAT_TYPE, JSON_FORMAT_TYPE)

# Supported handler types.
_TERMINAL_HANDLER = 0
_FILE_HANDLER = 1

# Custom logging level.
STEP_LOG_LEVEL_NAME = 'STEP'
STEP_LOG_LEVEL_VALUE = 25


def _set_formatter(handler_type, output_fmt=TEXT_FORMAT_TYPE):
    """Create a formatter with the defined output format for logging.

    This function returns a Formatter object set with the appropriate output
    format. An UtilError exception is raised if an unsupported output format
    is specified.

    :param handler_type: Target handler type to set the formatter.
    :type handler_type:  0 - terminal handler, 1 - file handler
    :param output_fmt:   Output format to log messages. Supported values.
                         By default: 'TEXT'.
    :type output_fmt:   'TEXT' or 'JSON'

    :return: Formatter object for the specified output format.

    :raises GadgetLogError: if an unsupported output format or handler type is
                            specified.
    """
    output_fmt = output_fmt.upper()
    if output_fmt in OUTPUT_FORMAT_TYPES:
        if output_fmt == JSON_FORMAT_TYPE:
            # Set JSON message format (same for terminal or file).
            return JsonFormatter(JSON_MSG_FORMAT, DEFAULT_DATE_FORMAT)
        else:
            if handler_type == _TERMINAL_HANDLER:
                # Set default (TEXT) message format for terminal.
                # Note: Custom Formatter used to allow different message to be
                # printed by logging level.
                return MessageByLoggingLevelFormatter(DEFAULT_TERM_MSG_FORMAT)
            elif handler_type == _FILE_HANDLER:
                # Set default (TEXT) message format for file.
                return logging.Formatter(DEFAULT_FILE_MSG_FORMAT,
                                         DEFAULT_DATE_FORMAT)
            else:
                raise GadgetLogError("Unknown logging handler type. Expected "
                                     "values: 0 for terminal and 1 for file.")
    else:
        raise GadgetLogError(
            "Unsupported output format for logging. Supported values: "
            "{0}".format(','.join(OUTPUT_FORMAT_TYPES)))


def _setup_terminal_handler(logging_level, output_fmt=TEXT_FORMAT_TYPE):
    """Setup the logging handler for the terminal.

    This method setup the logging handler to send information to the terminal.
    In particular, error messages (with the logging level ERROR and CRITICAL)
    are streamed to the stderr and the messages of the remaining levels to the
    stdout.

    :param logging_level: Logging level to set for the terminal handler.
                          See Python `logging` module for more information.
    :type logging_level:  Integer.
    :param output_fmt:    Output format to log messages. Supported values.
                          By default: 'TEXT'.
    :type output_fmt:     'TEXT' or 'JSON'
    """
    # Set message format for terminal.
    formatter = _set_formatter(_TERMINAL_HANDLER, output_fmt)

    # Filter to allow message to be skipped for the terminal.
    skip_term_filter = SkippAttributeFilter('skip_term')

    # Setup stdout terminal handler.
    stdout_hdl = logging.StreamHandler(stream=sys.stdout)
    stdout_hdl.setLevel(logging_level)
    stdout_hdl.setFormatter(formatter)
    filter_errors = FilterLoggingLevel(logging.ERROR, lower=False)
    stdout_hdl.addFilter(filter_errors)
    stdout_hdl.addFilter(skip_term_filter)
    logging.getLogger('').addHandler(stdout_hdl)

    # Setup stderr terminal handler.
    stderr_hdl = logging.StreamHandler(stream=sys.stderr)
    stderr_hdl.setLevel(logging_level)
    stderr_hdl.setFormatter(formatter)
    filter_non_errors = FilterLoggingLevel(logging.ERROR, lower=True)
    stderr_hdl.addFilter(filter_non_errors)
    stderr_hdl.addFilter(skip_term_filter)
    logging.getLogger().addHandler(stderr_hdl)


def _setup_file_handler(logging_level, log_filename,
                        output_fmt=TEXT_FORMAT_TYPE):
    """Setup a logging handler for a file.

    This method setup a logging handler to log message to a file. If the file
    does not exist it will be created otherwise log message will be appended
    to the end of the existing file.

    Note: Appropriate access privileges must be granted to the specified
    log file.

    :param logging_level: Logging level to set for the file handler.
                          See Python `logging` module for more information.
    :type logging_level:  Integer.
    :param log_filename:  Name/location of the file to use for logging.
    :type log_filename:   Filename
    :param output_fmt:    Output format to log messages. Supported values.
                          By default: 'TEXT'.
    :type output_fmt:    'TEXT' or 'JSON'
    """
    # Set message format for file.
    formatter = _set_formatter(_FILE_HANDLER, output_fmt)

    # Filter to allow message to be skipped for the file.
    skip_file_filter = SkippAttributeFilter('skip_file')

    # Setup file handler.
    file_hdl = logging.FileHandler(log_filename)
    file_hdl.setLevel(logging_level)
    file_hdl.setFormatter(formatter)
    file_hdl.addFilter(skip_file_filter)
    logging.getLogger().addHandler(file_hdl)


def setup_logging(program, verbosity=0, quiet=False, log_filename=None,
                  output_fmt=TEXT_FORMAT_TYPE):
    """Setup logging for MySQL Orchestrator Gadgets.

    This method setup the logging configurations for the terminal and/or file,
    according to the provided input parameters.

    :param program:      Name of the program (i.e., gadget) to setup logging.
    :type program:       String
    :param verbosity:    Verbosity level used to setup the logging level.
                         If > 0 then DEBUG otherwise INFO. By default 0.
    :type verbosity:     Integer
    :param quiet:        Boolean value indicating if the terminal logging will
                         be disable. By default False, terminal log enabled.
    :type quiet:         True or False
    :param log_filename: Indicate if file logging is enable and the name of the
                         target file. By default None, meaning that file
                         logging is disabled. If an empty string is used then
                         it will be assumed that the file name is the same of
                         the program with the 'log' extension and located in
                         the working directory.
    :type log_filename:  filename
    :param output_fmt:   Output format to log messages. Supported values.
                         By default: 'TEXT'.
    :type output_fmt:   'TEXT' or 'JSON'

    :raises GadgetLogError: if an error occur while trying to open the log
                            file.
    """
    # Set Logger class to use (with custom logging level for STEP).
    logging.setLoggerClass(CustomLevelLogger)
    # Determine the logging level to use based on verbosity.
    # Note: must to be set at the logger level otherwise default is used.
    logging_level = logging.DEBUG if verbosity else logging.INFO
    logging.getLogger().setLevel(logging_level)
    # Add a NullHandler at the root logger to make sure nothing is logged if
    # no other logging handler is set.
    logging.getLogger().addHandler(logging.NullHandler())

    # Setup file logging.
    if log_filename is not None:
        # logging.basicConfig()
        if log_filename == '':
            # Use default filename.
            log_filename = '{0}.log'.format(program)
        try:
            _setup_file_handler(logging_level, log_filename, output_fmt)
        except IOError as err:
            raise GadgetLogError("Error opening log file: "
                                 "{0}".format(str(err.args[1])))

    # Setup terminal logging.
    if not quiet:
        _setup_terminal_handler(logging_level, output_fmt)


def disable_logging():
    """Disable logging for MySQL Orchestrator Gadgets.

    This method disables logging by removing all handlers except the Null
    handler.
    """
    # Flush all logging messages.
    logging.shutdown()

    # Determine list of handlers to remove: all except the NullHandler.
    handlers_to_remove = []
    for handler in logging.getLogger().handlers:
        if not isinstance(handler, logging.NullHandler):
            handlers_to_remove.append(handler)

    # Remove all handlers effectively.
    # Note: cannot be done while iterating over the logger handlers.
    for handler in handlers_to_remove:
        logging.getLogger().removeHandler(handler)


def set_log_level(logging_level):
    """ Change the logging level for MySQL Orchestrator Gadgets.

    This method set an new logging level (for the root logger) and all message
    with a lower level will be ignored.

    :param logging_level: New logging level to use.
                          See Python `logging` module for more information.
    :type logging_level:  Integer
    """
    logging.getLogger().setLevel(logging_level)


class FilterLoggingLevel(logging.Filter):
    """Class to filter logging records according their logging level.

    This class is used to filter logging messages according to the logging
    level. It filters records with a lower or upper level based on the given
    reference logging level and a direction flag.
    """

    def __init__(self, level, lower=True):  # pylint: disable=W0231
        """ Constructor.

        :param level: Logging level to use as reference to filter records.
                      See Python `logging` module for more information.
        :type level:  Integer
        :param lower: Boolean value indicating the direction to filter records.
                      if True then records with a level < than the
                      reference level will be filtered otherwise records with
                      a level >= the input level are ignored. By default True,
                      messages with a lower level will be filtered.
        :type lower: True or False
        """
        self.level = level
        self.lower = lower

    def filter(self, record):
        """Filter logging record.

        :param record: Logging record to apply the filtering rule.

        :return: Boolean value indicating if the record is filtered (False) or
                 not (True).
        """
        if self.lower:
            # Filter records with a lower level.
            return record.levelno >= self.level
        else:
            # Filter records with a greater or equal level.
            return record.levelno < self.level


class SkippAttributeFilter(logging.Filter):
    """Filter class to skip logging records based on an extra attribute.

    This class is used to allow logging records to be skipped based on a
    predefined extra attribute. If the attribute is passed through the extra
    dictionary and has the value True then the record is filtered (skipped).
    """

    def __init__(self, skip_attribute):  # pylint: disable=W0231
        """ Constructor.

        :param skip_attribute: Name of the attribute (key) to use in the extra
                               dict to determine if the record will be filtered
                               or not. (if defined and value is True).
        :type skip_attribute: String
        """
        self.skip_attribute = skip_attribute

    def filter(self, record):
        """Filter logging record based on the predefined skip attribute.

        If the skip attribute is defined in the extra dictionary and its
        value is True, then the record is filtered.
        Note: Value defined in the extra dictionary are available directly
        as a logging record attribute.

        :param record:  Logging record to apply the filtering rule.
        :type record:   logging.LogRecord

        :return: Value indicating if the record is filtered (False) or not
                 (True).
        :rtype: boolean
        """
        return not getattr(record, self.skip_attribute, False)


class MessageByLoggingLevelFormatter(logging.Formatter):
    """Formatter class to use different message formats by logging level.

    This class is used to allow different messages formats to be used
    depending on the logging level. More specifically, it adds the prefixes
    'CRITICAL: ', 'ERROR: ', and 'WARNING: ' for messages with the CRITICAL,
    ERROR and WARNING logging levels respectively, independently of the
    initially specified string format.
    """

    def format(self, record):
        """ Format the specified record based on its logging level.

        This function formats the logging record differently based on the
        logging level. In particular, it adds the prefixes 'CRITICAL: ',
        'ERROR: ', and 'WARNING: ' automatically for messages with the level
        CRITICAL, ERROR and WARNING respectively.

        :param record:  Logging record to format.
        :type record:   logging.LogRecord

        :return: resulting formatted message.
        :rtype:  string
        """
        # Call the original format() function from the base class.
        res_str = super(MessageByLoggingLevelFormatter, self).format(record)

        # Add the prefix to the message based on the logging level.
        if record.levelno == logging.CRITICAL:
            return "CRITICAL: {0}".format(res_str)
        elif record.levelno == logging.ERROR:
            return "ERROR: {0}".format(res_str)
        elif record.levelno == logging.WARNING:
            return "WARNING: {0}".format(res_str)
        else:
            return res_str


class JsonFormatter(logging.Formatter):
    """JSON Formatter class.

    This class is used to format JSON logging messages and handle special
    characters.
    """

    def format(self, record):
        """ Format the specified record for JSON support.

        This function formats the logging record in JSON, handling some
        control characters properly, like \" and \\.

        Note: Use of other control character is not supported (ignored).
              Also, the use of dictionary as argument for the log message
              is not supported, i.e. no custom character substitution is
              performed for the arguments in that case.

        :param record:  Logging record to format.
        :type record:   logging.LogRecord

        :return: resulting formatted message.
        :rtype:  string
        """
        # First check if the log record was already handled, to avoid changing
        # it again if multiple logging handler are used.
        handled = getattr(record, "custom_json_handling", False)
        if not handled:
            # Set custom attribute to control already handled records.
            setattr(record, "custom_json_handling", True)
            # Replace \ by \\ and " by \" for the log record string arguments.
            replaced = False
            new_args = ()
            str_type = basestring if PY2 else str
            # A mapping can be specified as a log message arg but it is
            # handled in a different way internally, being assigned directly
            # to record.args. In that case, the record args are not processed.
            if not isinstance(record.args, collections.Mapping):
                for arg in record.args:
                    if isinstance(arg, str_type):
                        arg = arg.replace('\\', '\\\\')  # \ -> \\
                        arg = arg.replace('"', '\\"')  # " -> \"
                        replaced = True
                    new_args += (arg,)
                if replaced:
                    record.args = new_args
            # Replace \ by \\ and " by \" for the log record message.
            record.msg = record.msg.replace('\\', '\\\\')   # \ -> \\
            record.msg = record.msg.replace('"', '\\"')  # " -> \"

        # Call the original format() function from the base class and return
        # the result.
        return super(JsonFormatter, self).format(record)


class CustomLevelLogger(logging.getLoggerClass()):
    """Logger class with custom logging level.

    This logger class defines a custom logging level 'STEP' with a logging
    value greater than INFO and lower than WARNING.
    """
    def __init__(self, name, level=logging.NOTSET):
        """ Constructor.

        :param name:  Name of the logger.
        :type  name:  string
        :param level: Initial logging level for the logger. By default, NOTSET.
        :type level:  string
        """
        super(CustomLevelLogger, self).__init__(name, level)
        logging.addLevelName(STEP_LOG_LEVEL_VALUE, STEP_LOG_LEVEL_NAME)

    def step(self, msg, *args, **kwargs):
        """Log 'msg % args' with custom logging level 'STEP'.

        This method works similarly the default logging.Logger methods debug(),
        info(), warning(), error() and critical().
        For more information see: 'logging.Logger'.
        """
        if self.isEnabledFor(STEP_LOG_LEVEL_VALUE):
            self._log(STEP_LOG_LEVEL_VALUE, msg, args, **kwargs)
