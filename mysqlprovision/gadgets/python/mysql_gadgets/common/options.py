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
This module contains methods to add common option arguments among the multiple
gadgets.

"""
from __future__ import print_function
import argparse
import getpass
import os
import re
import sys

from mysql_gadgets import LICENSE_FRM, VERSION_FRM
from mysql_gadgets.common.connection_parser import parse_connection
from mysql_gadgets.common.group_replication import REX_UUID
from mysql_gadgets.common.group_replication import (GR_SSL_DISABLED,
                                                    GR_SSL_REQUIRED)
from mysql_gadgets.exceptions import GadgetCnxFormatError

_STORE_PASSWORD_LIST = "store_password_list"
_APPEND_PASSWORD_LIST = "append_password_list"


class GadgetsArgumentParser(argparse.ArgumentParser):
    """ This class customize the ArgumentParser for the Gadgets.

    Some customization are required to the ArgumentParser parser to align
    some option and the help output with the one of other MySQL tools.
    This class extends the ArgumentParser in order to customize some
    properties and behaviours to meet that goal.

    For example the format of the usage message was changed to display an
    initial welcome message with the copyright information.
    """

    def __init__(self, prog=None, usage=None, description=None, epilog=None,
                 parents=None, formatter_class=argparse.HelpFormatter,
                 prefix_chars='-', fromfile_prefix_chars=None,
                 argument_default=None, conflict_handler='error',
                 add_help=True):
        """Constructor.

        :param prog: The name of the program (default: sys.argv[0]),
                     same as for `argparse.ArgumentParser`.
        :type prog: string
        :param usage: A message describing the program usage (default:
                      generated from arguments added to parser),
                      same as for `argparse.ArgumentParser`.
        :type usage: string
        :param description: Text to display before the argument help
                            (default: none),
                            same as for `argparse.ArgumentParser`.
        :type description: string
        :param epilog: Text to display after the argument help (default: none),
                       same as for `argparse.ArgumentParser`.
        :type epilog: string
        :param parents: Parsers whose arguments should be included,
                        same as for `argparse.ArgumentParser`.
        :type parents: list of argparse.ArgumentParser objects
        :param formatter_class: A class for customizing the help output,
                                same as for `argparse.ArgumentParser`.
        :type formatter_class: class
        :param prefix_chars: Characters that prefix optional arguments
                             (default: '-'),
                             same as for `argparse.ArgumentParser`.
        :type prefix_chars: string
        :param fromfile_prefix_chars: Characters that prefix files from which
                                      additional arguments should be read
                                      (default: None),
                                      same as for `argparse.ArgumentParser`.
        :type fromfile_prefix_chars: string
        :param argument_default: The global default value for arguments
                                 (default: None),
                                 same as for `argparse.ArgumentParser`.
        :type argument_default: string
        :param conflict_handler: The strategy for resolving conflicting
                                 optionals (usually unnecessary),
                                 same as for `argparse.ArgumentParser`.
        :type conflict_handler: string
        :param add_help: Add a -?/--help option to the parser (default: True).
        :type add_help: boolean
        """
        if parents is None:
            parents = []
        # Call constructor of base class.
        if isinstance(argparse.ArgumentParser, type):
            # New style class
            super(GadgetsArgumentParser, self).__init__(
                prog=prog, usage=usage, description=description, epilog=epilog,
                parents=parents, formatter_class=formatter_class,
                prefix_chars=prefix_chars,
                fromfile_prefix_chars=fromfile_prefix_chars,
                argument_default=argument_default,
                conflict_handler=conflict_handler, add_help=False)
        else:
            # Old style class
            argparse.ArgumentParser.__init__(
                self, prog=prog, usage=usage, description=description,
                epilog=epilog, parents=parents,
                formatter_class=formatter_class, prefix_chars=prefix_chars,
                fromfile_prefix_chars=fromfile_prefix_chars,
                argument_default=argument_default,
                conflict_handler=conflict_handler, add_help=False)

        # Set welcome message for help.
        # Note: 'prog' can include name of commands if subparsers are used
        # (e.g., main_program command), that need to be ignored to only
        # display the main program name in the welcome message.
        program = self.prog.split()[0]
        self.welcome_message = LICENSE_FRM.format(program=program)

        # Add help if necessary, using -? (like for existing MySQL client
        # tools) instead of -h (by default for ArgumentParser).
        self.add_help = add_help
        if self.add_help:
            self.add_argument('-?', '--help',
                              action='help', default=argparse.SUPPRESS,
                              help='Show this help message and exit.')

    def format_help(self):
        """Format the help text.

        Overwrite the format_help() from the base class to add a welcome
        message (with the version and copyright information) at the beginning.

        Note: This welcome message is added to align the help output with the
        one from other MySQL server tools.

        :return: Formatted help text according to parser settings.
        :rtype: string
        """
        formatter = self._get_formatter()

        # print welcome message first.
        formatter.add_text(self.welcome_message)

        # usage
        formatter.add_usage(self.usage, self._actions,
                            self._mutually_exclusive_groups)

        # description
        formatter.add_text(self.description)

        # positionals, optionals and user-defined groups
        for action_group in self._action_groups:
            formatter.start_section(action_group.title)
            formatter.add_text(action_group.description)
            # pylint: disable=W0212
            formatter.add_arguments(action_group._group_actions)
            formatter.end_section()

        # epilog
        formatter.add_text(self.epilog)

        # determine help from format above
        return formatter.format_help()


class _ConnectionInfoStore(argparse.Action):
    """Class to be used as a ArgumentParser action type.
    It is similar to the default store action but it also does the parsing
    and validation of the connection string received as argument.
    """
    def __init__(self, **kwargs):
        # get ask_pass parameter from kwargs and remove it from the list of
        # arguments since it is not a valid argument for the constructor
        self.ask_pass = kwargs.pop("ask_pass")
        super(_ConnectionInfoStore, self).__init__(**kwargs)

    def __call__(self, parser_, namespace, values, option_string=None):
        try:
            param_dict = parse_connection(values)
            setattr(namespace, self.dest, param_dict)
            if self.ask_pass:
                # get the existing list of passwords to ask
                password_list = getattr(namespace, _STORE_PASSWORD_LIST, [])
                # add (argument_name, values ) tuple to the list
                password_list.append((self.dest, values))
                setattr(namespace, _STORE_PASSWORD_LIST, password_list)
        except GadgetCnxFormatError as e:
            parser_.error(e.errmsg)


class _ConnectionInfoAppend(argparse.Action):
    """Class to be used as a ArgumentParser action type.
    It is similar to the built-in append action but it also does the parsing
    and validation of the connection strings received as arguments.
    """
    def __init__(self, **kwargs):
        # get ask_pass parameter from kwargs and remove it from the list of
        # arguments since it is not a valid argument for the constructor
        self.ask_pass = kwargs.pop("ask_pass")
        super(_ConnectionInfoAppend, self).__init__(**kwargs)

    def __call__(self, parser_, namespace, values, option_string=None):
        try:
            param_dict = parse_connection(values)
            # Retrieve the set of already parsed connection strings and
            # check throw an error if the connection value is repeated.
            cache_name = "_{0}_cache".format(self.dest)
            # param_dict is a dictionary so it is not hashable.
            # we need a hashable representation of the connection parameters
            # to check if we've passed the same connection string more than
            # once.
            hashable_params = tuple(sorted(param_dict.items()))
            conn_list_cache = getattr(namespace, cache_name, set())
            if hashable_params in conn_list_cache:
                parser_.error("Cannot pass the same connection value more "
                              "than once: {0}".format(values))

            conn_list_cache.add(hashable_params)
            setattr(namespace, cache_name, conn_list_cache)
            # Not a repeated value, so add it to the list of connections
            conn_list = getattr(namespace, self.dest, None)
            conn_list = [] if conn_list is None else conn_list
            conn_list.append(param_dict)
            setattr(namespace, self.dest, conn_list)

            if self.ask_pass:
                # get the existing list of passwords to ask
                password_list = getattr(namespace, _APPEND_PASSWORD_LIST, [])
                # add (argument_name, values ) tuple to the list
                password_list.append((self.dest, values))
                setattr(namespace, _APPEND_PASSWORD_LIST, password_list)
        except GadgetCnxFormatError as e:
            parser_.error(e.errmsg)


class _UserInfoStore(argparse.Action):
    """Class to be used as a ArgumentParser action type.
    It is similar to the default store action but it also does the parsing
    and validation of the connection string received as argument.
    """
    def __init__(self, **kwargs):
        # get ask_pass parameter from kwargs and remove it from the list of
        # arguments since it is not a valid argument for the constructor
        self.ask_pass = kwargs.pop("ask_pass")
        self.def_user_name = kwargs.pop("def_user_name")
        super(_UserInfoStore, self).__init__(**kwargs)

    def __call__(self, parser_, namespace, values, option_string=None):
        if not values or values is None:
            values = self.def_user_name
        user_info = {"user": values}
        setattr(namespace, self.dest, user_info)
        if self.ask_pass:
            # get the existing list of passwords to ask
            password_list = getattr(namespace, _STORE_PASSWORD_LIST, [])
            # add (argument_name, values ) tuple to the list
            password_list.append((self.dest, values))
            setattr(namespace, _STORE_PASSWORD_LIST, password_list)


class _UUIDInfoStore(argparse.Action):
    """Class to be used as a ArgumentParser action type.
    It is similar to the default store action but it also does the parsing
    and validation of the connection string received as argument.
    """

    def __call__(self, parser_, namespace, values, option_string=None):
        msg = ("The value '{0}' given on {1} option is not a valid UUID"
               "".format(values, option_string))
        if re.match(REX_UUID, values) is None:
            parser_.error(msg)
        setattr(namespace, self.dest, values)


class _SSLStore(argparse.Action):
    """Class to be used as a ArgumentParser action type.
    It is similar to the default store action but it also does the parsing
    and validation of the connection string received as argument.
    """

    def __call__(self, parser_, namespace, value, option_string=None):
        if not os.path.exists(value):
            parser_.error("the given path '{0}' in option {1} does not "
                          "exist or can not be accessed"
                          "".format(value, option_string))

        if not os.path.isfile(value):
            parser_.error("the given path '{0}' in option {1} does not "
                          "correspond to a file"
                          "".format(value, option_string))

        setattr(namespace, self.dest, value)


def add_store_connection_option(parser, option_name_list, dest, help_txt=None,
                                required=True, ask_pass=True,
                                add_ssl_opts=True):
    """Adds a connection option to the parser passed as parameter.
    The added option provides a similar behavior to the default store action,
    however it also parses and validates the connection string it receives and
    turns it into a connection dictionary.

    :param parser: instance of ArgumentParser to which we want to add the
                   server argument
    :type parser: argparse.ArgumentParser
    :param option_name_list: list of command-line option strings which should
                             be associated with the argument.
    :type option_name_list: list
    :param dest: The name of the attribute that we can use to access the value
                 of the argument.
    :type dest: str
    :param help_txt: the help string describing the argument.
    :type help_txt: str
    :param required: if set to True, this argument will be mandatory, else set
                     to False it will be optional. By default True.
    :type required: bool
    :param ask_pass: If True, we will add this connection to the list of
                     connections to which passwords need to be asked.
    :type ask_pass: bool
    :param add_ssl_opts: If set to True, add the options to store the SSL
                         certificates for the connection. By Default True.
    :type add_ssl_opts: bool
    """
    if help_txt is None:
        help_txt = ("Connection information in the form: <user>@<host>"
                    "[:<port>].")
    parser.add_argument(*option_name_list, dest=dest, help=help_txt,
                        action=_ConnectionInfoStore, required=required,
                        ask_pass=ask_pass)

    if add_ssl_opts:
        help_suffix = (" Used to connect to the server given on {0} option."
                       "".format(option_name_list[0]))
        _add_ssl_connection_options(parser, option_name_list[0], dest,
                                    help_suffix=help_suffix)


def add_append_connection_option(parser, option_name_list, dest,
                                 help_txt=None, required=True,
                                 ask_pass=True):
    """Adds a connection option to the parser passed as parameter.
    The added option provides a similar behavior to the default append action,
    it can be used several times to store several values. The added option
    also parses and validates the connection strings it receives and turns them
    into connection dictionaries.

    :param parser: instance of ArgumentParser to which we want to add the
                   server argument
    :type parser: argparse.ArgumentParser
    :param option_name_list: list of command-line option strings which should
                             be associated with the argument.
    :type option_name_list: list
    :param dest: The name of the attribute that we can use to access the value
                 of the argument.
    :type dest: str
    :param help_txt: the help string describing the argument.
    :type help_txt: str
    :param required: if set to True, this argument will be mandatory, else if
                     set to False it will be optional. By default True.
    :type required: bool
    :param ask_pass: If True, we will add this connection to the list of
                     connections to which passwords need to be asked.
    :type ask_pass: bool
    """
    if help_txt is None:
        help_txt = ("Connection information in the form: <user>@<host>"
                    "[:<port>]. Repeat the option to specify several values.")
    parser.add_argument(*option_name_list, dest=dest, help=help_txt,
                        action=_ConnectionInfoAppend, required=required,
                        ask_pass=ask_pass)


def add_store_user_option(parser, option_name_list, dest, def_user_name="",
                          help_txt=None, required=True, ask_pass=True):
    """Adds a user option to the parser passed as parameter.
    The added option provides a similar behavior to the default store action,
    however it also request the password of the user.

    :param parser: instance of ArgumentParser to which we want to add the
                   server argument
    :type parser: argparse.ArgumentParser
    :param option_name_list: list of command-line option strings which should
                             be associated with the argument.
    :type option_name_list: list
    :param dest: The name of the attribute that we can use to access the value
                 of the argument.
    :type dest: str
    :param def_user_name: The default user name used if not provided.
    :type def_user_name: str
    :param help_txt: the help string describing the argument.
    :type help_txt: str
    :param required: if set to True, this argument will be mandatory, else set
                     to False it will be optional. By default True.
    :type required: bool
    :param ask_pass: If True, we will add this connection to the list of
                     connections to which passwords need to be asked.
    :type ask_pass: bool
    """
    if help_txt is None:
        help_txt = "User information in the form: <user>@<host>"
    parser.add_argument(*option_name_list, dest=dest, help=help_txt,
                        action=_UserInfoStore, required=required,
                        ask_pass=ask_pass, def_user_name=def_user_name,
                        default=def_user_name)


def read_passwords(namespace):
    """Ask for the passwords, either via tty or stdin depending on the
    flag to read passwords via stdin and updates namespace the passwords

    :param namespace: namespace object from which we will retrieve the list of
                      passwords to read.
    :type namespace: argparse.Namespace
    """
    store_password_list = getattr(namespace, _STORE_PASSWORD_LIST, None)
    append_password_list = getattr(namespace, _APPEND_PASSWORD_LIST, None)
    if store_password_list is not None:
        for opt_name, val in store_password_list:
            prompt = ("Enter the password for {0} ({1}): "
                      "".format(opt_name, val))
            if getattr(namespace, "stdin_pw", None):
                print(prompt)
                getattr(namespace, opt_name)["passwd"] = \
                    sys.stdin.readline().rstrip(os.linesep)
            else:
                getattr(namespace, opt_name)["passwd"] = \
                    getpass.getpass(prompt)

    if append_password_list is not None:
        for index, (opt_name, val) in enumerate(append_password_list):
            prompt = ("Enter the password for {0} ({1}): "
                      "".format(opt_name, val))
            if getattr(namespace, "stdin_pw", None):
                print(prompt)
                getattr(namespace, opt_name)[index]["passwd"] = \
                    sys.stdin.readline().rstrip(os.linesep)
            else:
                getattr(namespace, opt_name)[index]["passwd"] = \
                    getpass.getpass(prompt)


def force_read_password(namespace, destination, value):
    """Register a load for a password.

    This is a workaround to the argparse.Actions not being called when the
    default value is used.

    :param namespace: The namespaces with the options
    :type namespace: dict
    :param destination: The name of the attribute used to save password.
    :type destination: string
    :param value: The value used to request the password
    :type value: string
    """
    # get the existing list of passwords to ask
    password_list = getattr(namespace, _STORE_PASSWORD_LIST, [])

    # If the default value was used, the __call__ method of the action will
    # not be called.
    # Any passwords we need to read associated with that option need to be
    # forcefully added to the list of passwords to read on the namespace.
    found = False
    # Search on the password list that will be ask from the user, to see if
    # the password for this option is already listed.
    for opt_name, _ in password_list:
        if opt_name == destination:
            # Mark as found to prevent requiring it again,
            found = True

    if not found:
        # Option wasn't found in the list, it requires to be add.
        # Adding (argument_name, values) tuple to the password_list.
        password_list.append((destination, value))
        setattr(namespace, _STORE_PASSWORD_LIST, password_list)


def read_extra_password(prompt, read_from_stdin):
    """Reads extra password that is not associated with any option.
     Reads a password that isn't associated with any argparse option and
     returns the value read.

    :param prompt: prompt that will be used to ask for the password
    :type prompt: str
    :param read_from_stdin: If True reads password from stdin,
                            else from terminal
    :type read_from_stdin: bool
    """
    if read_from_stdin:
        print(prompt)
        return sys.stdin.readline().rstrip(os.linesep)

    else:
        return getpass.getpass(prompt)


def add_stdin_password_option(parser, help_txt=None,
                              required=False):
    """Adds an argument to the parser to read passwords from stdin.

    :param parser:   instance of ArgumentParser to which we want to add the
                     the argument.
    :type parser:    argparse.ArgumentParser
    :param help_txt: the help string describing the argument.
    :type help_txt:  str
    :param required: if set to True, this argument will be mandatory, else set
                     to False it will be optional. By default False.
    :type required:  boolean
    """
    if help_txt is None:
        help_txt = ("Read passwords from the standard input instead of from "
                    "the terminal.")
    parser.add_argument("--stdin", dest="stdin_pw",
                        help=help_txt, action="store_true", required=required)


def add_verbose_option(parser, dest="verbose", help_txt=None):
    """Adds a verbose argument to the parser.

    :param parser:   instance of ArgumentParser to which we want to add the
                     the argument.
    :type parser:    argparse.ArgumentParser
    :param dest:     The name of the attribute that we can use to access the
                     value of the argument. By default "verbose".
    :type dest:      str
    :param help_txt: the help string describing the argument.
    :type help_txt:  str
    """
    if help_txt is None:
        help_txt = ("Show more detailed information during the execution of "
                    "the program.")
    parser.add_argument("--verbose", "-v", dest=dest, help=help_txt,
                        action="store_true")


def add_version_option(parser):
    """Adds a version option to the parser passed as parameter.

    :param parser: instance of ArgumentParser to which we want to add the
                   option
    :type parser:  argparse.ArgumentParser
    """
    help_txt = "Show version and exit"
    parser.add_argument("--version", "-V", help=help_txt, action="version",
                        version=VERSION_FRM.format(program=parser.prog))


def add_expected_version_option(parser):
    """Adds a expected-version option to the parser passed as parameter.

    :param parser: Instance of ArgumentParser to which we want to add the
                   option.
    :type parser:  argparse.ArgumentParser
    """
    help_txt = ("Expected version of the tool. The value must be compatible "
                "with the current version to be able to run any command.")
    parser.add_argument("-xV", "--expected-version", help=help_txt,
                        dest='expected_version', action="store")


def add_uuid_option(parser, option_name, dest, help_txt, required=False):
    """Adds a UUID option to the parser passed as parameter.

    :param parser: Instance of ArgumentParser to which we want to add the
                   option
    :type parser:  argparse.ArgumentParser
    :param option_name: The command-line name option string which should
                        be associated with the argument.
    :type option_name: string
    :param dest: The name of the attribute that we can use to access the value
                 of the argument.
    :type dest: str
    :param help_txt: The help string describing the argument.
    :type help_txt: str
    :param required: If set to True, this argument will be mandatory, else set
                     to False it will be optional. By default False.
    :type required: bool
    """

    parser.add_argument(option_name, dest=dest, help=help_txt,
                        action=_UUIDInfoStore, required=required)


def add_port_option(parser, help_txt):
    """Adds port option to the parser passed as parameter.

    This add a port option following what is used by default for MySQL server
    tools --port and -P.

    :param parser: instance of ArgumentParser to which we want to add the
                   option
    :type parser:  argparse.ArgumentParser
    :param help_txt: The help string describing the argument.
    :type help_txt: str
    """
    parser.add_argument("--port", "-P", dest="port", help=help_txt,
                        required=True, type=int)


def add_ssl_options(parser, check_file_exist=False):
    """Adds ssl-ca, ssl-cert and ssl-key options to the given parser.

    This adds the ssl options for set the certificates to configure Group
    Replication.

    :param parser: instance of ArgumentParser to which we want to add the
                   ssl options
    :type parser:  argparse.ArgumentParser
    :param check_file_exist: determines if the check to verify if files
                             specified for the SSL options exist will be
                             performed or not. By default, False, meaning that
                             the file existence check will not be performed.
    :type check_file_exist:  bool
    """
    ssl_opts = [("--ssl-ca", "ssl_ca",
                 "Path of file that contains list of trusted SSL CAs."),
                ("--ssl-cert", "ssl_cert",
                 "Path of file that contains X509 certificate in PEM format."),
                ("--ssl-key", "ssl_key",
                 "Path of file that contains X509 key in PEM format.")]

    action = _SSLStore if check_file_exist else 'store'
    for opt_name, dest, help_txt in ssl_opts:
        parser.add_argument(opt_name, dest=dest, help=help_txt,
                            action=action)

    ssl_mode_help = ("Specifies the SSL mode that should be used "
                     "to configure the instance. By default: {0}"
                     "".format(GR_SSL_REQUIRED))

    parser.add_argument("--ssl-mode", dest="ssl_mode", help=ssl_mode_help,
                        action="store", type=str.upper,
                        default=GR_SSL_REQUIRED,
                        choices=[GR_SSL_REQUIRED, GR_SSL_DISABLED])


def _add_ssl_connection_options(parser, opt_prefix="instance",
                                var_prefix="instance", help_suffix="",
                                check_file_exist=False):
    """Add ssl-ca, ssl-cert and ssl-key options to the given parser.

    This adds the ssl options for set the certificates to configure Group
    Replication.

    :param parser: instance of ArgumentParser to which we want to add the
                   SSL options
    :type parser:  argparse.ArgumentParser
    :param opt_prefix: the option prefix to use for naming options. By default
                       'instance'.
    :type opt_prefix:  string
    :param var_prefix: value that will be used as the prefix for the SSL
                       attributes of the object that is returned by
                       ArgumentParser.parse_args.
    :type var_prefix: string
    :param help_suffix: Extra text that will added to the end of the help text
                        of each SSL option.
    :type help_suffix: string
    :param check_file_exist: determines if the check to verify if files
                             specified for the SSL options exist will be
                             performed or not. By default, False, meaning that
                             the file existence check will not be performed.
    :type check_file_exist:  bool
    """
    if len(help_suffix) > 0 and not help_suffix.startswith(" "):
        help_suffix = " {0}".format(help_suffix)

    ssl_opts = [("{0}-ssl-ca".format(opt_prefix),
                 "{0}_ssl_ca".format(var_prefix),
                 "Path of file that contains list of trusted SSL CAs."
                 "{0}".format(help_suffix)),
                ("{0}-ssl-cert".format(opt_prefix),
                 "{0}_ssl_cert".format(var_prefix),
                 "Path of file that contains X509 certificate in PEM format."
                 "{0}".format(help_suffix)),
                ("{0}-ssl-key".format(opt_prefix),
                 "{0}_ssl_key".format(var_prefix),
                 "Path of file that contains X509 key in PEM format."
                 "{0}".format(help_suffix))]

    action = _SSLStore if check_file_exist else 'store'
    for opt_name, dest, help_txt in ssl_opts:
        parser.add_argument(opt_name, dest=dest, help=help_txt,
                            action=action)
