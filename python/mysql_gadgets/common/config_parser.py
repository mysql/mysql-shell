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
Module to manage (read and write) MySQL option files.
"""

from __future__ import print_function
from collections import OrderedDict
import os
import re
import sys
import stat

from mysql_gadgets.exceptions import GadgetConfigParserError
from mysql_gadgets.common.tools import get_abs_path

PY2 = int(sys.version[0]) == 2
# pylint: disable=F0401
# pylint: disable=E0012, C0413, C0411
if PY2:
    from ConfigParser import (RawConfigParser, NoOptionError, NoSectionError,
                              Error, MissingSectionHeaderError,
                              ParsingError)
else:
    from configparser import (ConfigParser as RawConfigParser, NoOptionError,
                              NoSectionError, Error, MissingSectionHeaderError,
                              DuplicateSectionError, SectionProxy,
                              DuplicateOptionError)

DEFAULT_EXTENSIONS = {
    'nt': ('ini', 'cnf'),
    'posix': ('cnf',)
}

MYSQL_OPTCRE_NV = re.compile(
    r'\s*'  # any starting space/tab
    r'(?P<option>[^=]+?)'  # option (permissive like original but not greedy)
    r'\s*(?:'  # any number of space/tab, optionally followed by
    r'(?P<vi>[=])\s*'  # separator (=) and any spaces and
    r'(?P<value>((?P<quote>[\'"]).*?(?P=quote))|'  # value with quotes or
    r'([^#]*?)))?'  # value (all except '#'),
    r'\s*(?:'  # any number of space/tab, optionally followed by
    r'(?P<comment>#.*))?'  # comment starting with '#',
    r'$'  # up to eol
)

# MySQL programs read startup options from the following files, in the
# specified order.
# See: http://dev.mysql.com/doc/refman/5.7/en/option-files.html
DEFAULT_LOCATIONS = {
    'nt': (r"%PROGRAMDATA%\MySQL\MySQL Server 5.7\my.ini",
           r"%PROGRAMDATA%\MySQL\MySQL Server 5.7\my.cnf",
           r"%WINDIR%\my.ini", r"%WINDIR%\my.cnf",
           r"C:\my.cnf", r"C:\my.ini"),
    'posix': ("/etc/my.cnf", "/etc/mysql/my.cnf", "$MYSQL_HOME/my.cnf",
              "~/.my.cnf")
}


def option_list_to_dictionary(opt_list):
    """Converts a list of options to a dictionary.

    This function converts a list of option strings to a dictionary of options
    and respective values. E.g: ["port=13001", "binlog_format=ROW"] is
    transformed into {"port": "13001", "binlog_format": "ROW"}
    :param opt_list: list of option strings
    :type opt_list: list
    :return: dictionary with options and respective values.
    :rtype: dict

    Note: it converts hyphen '-' to underscores '_' on the value part.
    """
    res = {}
    for opt in opt_list:
        opt_val = opt.split("=", 1)
        opt_name = opt_val[0].strip().replace("-", "_")
        try:
            val = opt_val[1].strip()
        except IndexError:  # option without value
            val = None
        res[opt_name] = val
    return res


class MySQLOptionsParser(object):  # pylint: disable=R0901
    """This class implements methods to parse MySQL option files.

    Some properties for MySQL configuration files are different from the ones
    assumed by the Python implementation for the ConfigParser (Python 2)
    and configparser (Python 3) modules, requiring a custom implementation
    to handle them.

    Example of different properties:
    - Section (group) names are not case sensitive;
    - option names are case sensitive (like options in command line);
    - Only the '=' character is used to separate options with values;
    - By default, options without value are supported;
    - Only the '#' comment can start in the middle of a line;
    - Values can be optionally enclose within single or double quotation marks,
      which is useful if the value contains a '#' comment character;
    - specific escape sequences are supported;
    - there is no special 'default' section with precedence over other
      sections;
    - Specific directives are supported (!include and !includedir) to include
      configurations from other files.

    For more information, see:
    http://dev.mysql.com/doc/refman/5.7/en/option-files.html
    """

    # pylint: disable=R0901
    class MySQLRawConfigParser(RawConfigParser):
        """ This class customize the behaviour of the Python RawConfigParser.

        The default behaviour of the classes in the Python configparser module
        does not match the one used by MySQL to parse configuration files,
        requiring some properties and methods to be overwritten.
        For example, multiple line values are not supported for MySQL options
        files and should be disabled.
        """
        def __init__(self, **kargs):  # pylint: disable=E1002
            """Constructor.
            """
            # Call constructor of base class.
            if isinstance(RawConfigParser, type):
                # New style class
                super(MySQLOptionsParser.MySQLRawConfigParser, self).__init__(
                    **kargs)
            else:
                # Old style class
                RawConfigParser.__init__(self, **kargs)

            # Option names are case sensitive for MySQL and you can use '_' or
            # '-' interchangeably
            self.optionxform = lambda option: option.replace('-', '_')

            # Set regexp used to parse options to use only '=' as valid
            # separator and ignore space at the start of an option.
            self._optcre = MYSQL_OPTCRE_NV

        if PY2:
            # Overwrite _read() for Python 2 to remove multiline support.
            def _read(self, fp, fpname):
                """Copy of base class method without multiline value code.
                Note: other minor adjustments were also made to meet the
                coding standard and avoid pylint issues.
                """
                cursect = None  # None, or a dictionary
                optname = None
                lineno = 0
                e = None  # None, or an exception
                while True:
                    line = fp.readline()
                    if not line:
                        break
                    lineno += 1
                    # comment or blank line?
                    if line.strip() == '' or line[0] in '#;':
                        continue
                    if (line.split(None, 1)[0].lower() == 'rem' and
                            line[0] in "rR"):
                        # no leading whitespace
                        continue
                    # HERE: continuation line code (if ...) removed.
                    # a section header or option header?
                    else:
                        # is it a section header?
                        mo = self.SECTCRE.match(line)
                        if mo:
                            sectname = mo.group('header')
                            if sectname in self._sections:
                                cursect = self._sections[sectname]
                            # HERE, remove default section special handling.
                            else:
                                cursect = self._dict()
                                cursect['__name__'] = sectname
                                self._sections[sectname] = cursect
                            # So sections can't start with a continuation line
                            optname = None
                        # no section header in the file?
                        elif cursect is None:
                            raise MissingSectionHeaderError(fpname, lineno,
                                                            line)
                        # an option line?
                        else:
                            mo = self._optcre.match(line)
                            if mo:
                                optname, vi, optval = mo.group('option', 'vi',
                                                               'value')
                                optname = self.optionxform(optname.rstrip())
                                # This check is fine because the OPTCRE cannot
                                # match if it would set optval to None
                                if optval is not None:
                                    if vi in ('=', ':') and ';' in optval:
                                        # ';' is a comment delimiter only if
                                        # it follows a spacing character
                                        pos = optval.find(';')
                                        if (pos != -1 and
                                                optval[pos - 1].isspace()):
                                            optval = optval[:pos]
                                    optval = optval.strip()
                                    # allow empty values
                                    if optval == '""':
                                        optval = ''
                                    # pylint: disable=E0012,E1136
                                    cursect[optname] = [optval]
                                else:  # valueless option handling
                                    if cursect is not None:
                                        # pylint: disable=E0012,E1136
                                        cursect[optname] = optval
                            else:
                                # a non-fatal parsing error occurred. set up
                                # the exception but keep going. the exception
                                # will be raised at the end of the file and
                                # will contain a list of all bogus lines
                                if not e:
                                    e = ParsingError(fpname)
                                e.append(lineno, repr(line))
                # if any parsing errors occurred, raise an exception
                if e:
                    # pylint: disable=E0702
                    raise e

                # join the multi-line values collected while reading
                all_sections = [self._defaults]
                all_sections.extend(self._sections.values())
                for options in all_sections:
                    for name, val in options.items():
                        if isinstance(val, list):
                            options[name] = '\n'.join(val)

            # Override add_section method to remove hardcoded default_section
            # value
            def add_section(self, section):
                """Create a new section in the configuration.

                Raise DuplicateSectionError if a section by the specified name
                already exists. Raise ValueError if name is DEFAULT or any
                of it's
                case-insensitive variants.
                """
                # If condition from base class removed because we want to
                # allow a section with the name 'default' to be added.
                if section in self._sections:
                    raise DuplicateSectionError(section)
                self._sections[section] = self._dict()
        else:
            # Overwrite _read() for Python 3 to remove multiline support.
            def _read(self, fp, fpname):
                """Copy of base class method without multiline value code.

                Note: other minor adjustments were also made to meet the
                coding standard and avoid pylint issues.
                """
                elements_added = set()
                cursect = None  # None, or a dictionary
                sectname = None
                optname = None
                lineno = 0
                e = None  # None, or an exception
                for lineno, line in enumerate(fp, start=1):
                    comment_start = sys.maxsize
                    # strip inline comments
                    # pylint: disable=E1101
                    inline_prefixes = {p: -1 for p in
                                       self._inline_comment_prefixes}
                    while comment_start == sys.maxsize and inline_prefixes:
                        next_prefixes = {}
                        for prefix, index in inline_prefixes.items():
                            index = line.find(prefix, index + 1)
                            if index == -1:
                                continue
                            next_prefixes[prefix] = index
                            if index == 0 or (
                                    index > 0 and line[index - 1].isspace()):
                                comment_start = min(comment_start, index)
                        inline_prefixes = next_prefixes
                    # strip full line comments
                    for prefix in self._comment_prefixes:
                        if line.strip().startswith(prefix):
                            comment_start = 0
                            break
                    if comment_start == sys.maxsize:
                        comment_start = None
                    value = line[:comment_start].strip()
                    if not value:
                        if self._empty_lines_in_values:
                            # add empty line to the value, but only if there
                            # was no comment on the line
                            # pylint: disable=E0012,E1136
                            if (comment_start is None and
                                    cursect is not None and
                                    optname and
                                    cursect[optname] is not None):
                                cursect[optname].append(
                                    '')  # newlines added at join
                        continue
                    # continuation line?
                    # HERE: continuation line code (if ... else) removed.
                    # a section header or option header?
                    # is it a section header?
                    mo = self.SECTCRE.match(value)
                    if mo:
                        sectname = mo.group('header')
                        if sectname in self._sections:
                            if self._strict and sectname in elements_added:
                                raise DuplicateSectionError(sectname,
                                                            fpname,
                                                            lineno)
                            cursect = self._sections[sectname]
                            elements_added.add(sectname)
                        elif sectname == self.default_section:
                            cursect = self._defaults
                        else:
                            cursect = self._dict()
                            self._sections[sectname] = cursect
                            self._proxies[sectname] = SectionProxy(self,
                                                                   sectname)
                            elements_added.add(sectname)
                        # So sections can't start with a continuation line
                        optname = None
                    # no section header in the file?
                    elif cursect is None:
                        raise MissingSectionHeaderError(fpname, lineno,
                                                        line)
                    # an option line?
                    else:
                        mo = self._optcre.match(value)
                        if mo:
                            optname, _, optval = mo.group('option', 'vi',
                                                          'value')
                            if not optname:
                                e = self._handle_error(e, fpname, lineno,
                                                       line)
                            optname = self.optionxform(optname.rstrip())
                            if (self._strict and
                                    (sectname, optname) in elements_added):
                                raise DuplicateOptionError(sectname,
                                                           optname,
                                                           fpname, lineno)
                            elements_added.add((sectname, optname))
                            # This check is fine because the OPTCRE cannot
                            # match if it would set optval to None
                            if optval is not None:
                                optval = optval.strip()
                                # pylint: disable=E0012,E1136
                                cursect[optname] = [optval]
                            else:
                                # valueless option handling
                                if cursect is not None:
                                    # pylint: disable=E0012,E1136
                                    cursect[optname] = None
                        else:
                            # a non-fatal parsing error occurred. set up the
                            # exception but keep going. the exception will be
                            # raised at the end of the file and will contain a
                            # list of all bogus lines
                            e = self._handle_error(e, fpname, lineno, line)
                # if any parsing errors occurred, raise an exception
                if e:
                    raise e  # pylint: disable=E0702
                # pylint: disable=E1101
                self._join_multiline_values()

        # pylint: disable=E0203, W0201
        def convert_sections_to_lower(self):
            """ Convert all sections to lower cases.

            Note: Sections are case insensitive, therefore all converted to
            lower cases.

            :raises GadgetConfigParserError: If a duplicated section is found
            when converting to lower case.
            """
            # Convert all section names to lower case.
            new_sections = self._dict()
            for k in self._sections:
                v = self._sections[k]
                new_k = k.lower()
                if new_k in new_sections:
                    raise GadgetConfigParserError(
                        "File format error: section '{0}' is duplicated "
                        "(sections are case insensitive).".format(k))
                new_sections[new_k] = v
            self._sections = new_sections
            # Value for _proxies needs to be converted, only used for Python 3.
            if hasattr(self, "_proxies"):
                new_proxies = self._dict()
                for k in self._proxies:
                    v = self._proxies[k]
                    new_k = k.lower()
                    new_proxies[new_k] = v
                self._proxies = new_proxies

    def __init__(self, filename):  # pylint: disable=W0231
        """Constructor.

        :param filename: filename of the option file to read
        :type filename: string
        """

        if PY2:
            kwargs = {'allow_no_value': True}
            # Monkey patch ConfigParser DEFAULTSECT value, in order to handle
            # the 'default' section like any other (not as a special one).
            import ConfigParser
            ConfigParser.DEFAULTSECT = ''
        else:
            kwargs = {'allow_no_value': True, 'delimiters': ('=',),
                      'strict': False, 'empty_lines_in_values': False,
                      'interpolation': None, 'default_section': ''}

        # ConfigParser with information about configuration files included
        # from the configuration file specified as argument
        self._included_opt_parser = MySQLOptionsParser.MySQLRawConfigParser(
            **kwargs)
        # ConfigParser with information about the configuration file
        # specified as argument
        self._main_opt_parser = MySQLOptionsParser.MySQLRawConfigParser(
            **kwargs)

        self.default_extension = DEFAULT_EXTENSIONS[os.name]
        # get the absolute path for the provided filename
        self.filename = get_abs_path(filename, os.getcwd())
        self._parse_option_file(self.filename)

        # Convert all section names to lower case.
        self._main_opt_parser.convert_sections_to_lower()
        self._included_opt_parser.convert_sections_to_lower()

        # Flag that must be set if any changes were made to the config read
        # from the file
        self.modified = False

    def _parse_option_file(self, filename):
        """Parse the given options file.

        This method parses a valid MySQL option file. It supports !include and
        !includedir directives and also parses all files included by those
        directives.

        Sor more information, see:
        http://dev.mysql.com/doc/refman/5.7/en/option-files.html

        :param filename: Absolute path to the option file to parse.
        :type filename: str

        :raises GadgetConfigParserError: If the given file or any of the
        included files is not readable.
        """

        # Get files that need to be read, based on include directives.
        err_msg = "Option file '{0}' being included again in file '{1}'"
        files = [filename]
        for index, file_ in enumerate(files):
            try:
                with open(file_, 'r') as op_file:
                    for line in op_file:
                        if line.startswith('!includedir'):
                            _, dir_path = line.split(None, 1)
                            dir_path = dir_path.strip()
                            dir_path = get_abs_path(dir_path, file_)
                            for entry in os.listdir(dir_path):
                                entry = os.path.join(dir_path, entry)
                                if (os.path.isfile(entry) and entry.endswith(
                                        self.default_extension)):
                                    # Only process files with valid extension.
                                    if entry in files:
                                        raise GadgetConfigParserError(
                                            err_msg.format(entry, file_))
                                    files.insert(len(files), entry)
                                else:
                                    # Skip all other files or directories.
                                    continue
                        elif line.startswith('!include'):
                            _, filename = line.split(None, 1)
                            filename = filename.strip()
                            filename = get_abs_path(filename, file_)
                            if filename in files:
                                raise GadgetConfigParserError(err_msg.format(
                                    filename, file_))
                            files.insert(len(files), filename)
            except (IOError, OSError) as err:
                raise GadgetConfigParserError(
                    "Option file '{0}' is not readable: "
                    "'{1}'".format(self.filename, str(err)),
                    cause=err)

        # Read configurations from option files.
        parse_err = "File '{0}' could not be parsed: {1}"
        for index, file_ in enumerate(files):
            if index == 0:
                # main file is read to main parser only
                try:
                    self._main_opt_parser.read(file_)
                except Error as err:
                    raise GadgetConfigParserError(parse_err.format(file_,
                                                                   str(err)),
                                                  cause=err)
            else:
                # config files from !include or !includedir are only added to
                # the include parser
                try:
                    self._included_opt_parser.read(file_)
                except Error as err:
                    raise GadgetConfigParserError(parse_err.format(file_,
                                                                   str(err)),
                                                  cause=err)

    def sections(self):
        """Return a list of the sections available.

        :return: List of available options.
        :rtype: list
        """

        res = self._main_opt_parser.sections() + \
            self._included_opt_parser.sections()

        # Remove duplicates but preserve the order
        seen = set()
        return [x for x in res if not (x in seen or seen.add(x))]

    def add_section(self, section):
        """Add a new section (group) for the main option file.

        :param section: Name of the section (group) to add.
        :type section: str
        :raises GadgetConfigParserError: If the section already exists on
        the main option file.
        """
        # None or '' are not valid section names.
        if not section:
            raise GadgetConfigParserError("Cannot add an empty section.")
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        # If main config file has the section then raise an exception
        if self._main_opt_parser.has_section(section):
            raise GadgetConfigParserError(
                "Section '{0}' already exists.".format(section))
        else:  # Create it
            self._main_opt_parser.add_section(section)
            self.modified = True

    def has_section(self, section):
        """Indicate if the section (group) exists.

        This checks if the section exists in all read configuration, even
        for those read from files included with the !include or !includedir
        directives.

        :param section: Name of the section to check.
        :type section: str
        :return: True if the section exists False otherwise.
        :rtype: bool
        """
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        return self._included_opt_parser.has_section(section) or \
            self._main_opt_parser.has_section(section)

    def options(self, section):
        """Returns the list of options in the specified section (group).

        :param section: Name of the section (group) to get the options.
        :type section: str

        :return: All options available in the specified section.
        :rtype: list

        :raises GadgetConfigParserError: If the specified section does not
        exist.
        """
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        res = []
        no_section_included = False
        no_section_main = False
        try:
            res.extend(self._main_opt_parser.options(section))
        except NoSectionError:
            no_section_main = True
        try:
            res.extend(self._included_opt_parser.options(section))
        except NoSectionError:
            no_section_included = True

        # Raise error if the section is not found.
        # Note: not finding a section is different from finding it empty
        # (without any options).
        if no_section_included and no_section_main:
            raise GadgetConfigParserError("No section '{0}'.".format(section))

        # Remove duplicates but preserve the order
        seen = set()
        return [x for x in res if not (x in seen or seen.add(x))]

    def has_option(self, section, option):
        """Indicate if the given option exists in the section (group).

        :param section: Name of the section (group) for the option to check.
        :type section: str
        :param option: Name of the option to check.
        :type option: str

        :return: True if the section (group) exists and contains the given
                 option, False otherwise.
        :rtype: bool
        """
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        return self._included_opt_parser.has_option(section, option) or \
            self._main_opt_parser.has_option(section, option)

    def get(self, section, option):
        """Get the value for the given option and section.

        :param section: Name of the section (group).
        :type section: str
        :param option: Name of the option to get.
        :type option: str

        :return: Value of the option
        :rtype: str

        :raises GadgetConfigParserError: If the section does not exists or
        it does not contain the specified option.
        """
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        # Since included files have higher precedence, first retrieve the
        # value from the include_parser and if not found then try to
        # retrieve it from the main file.
        try:
            val = self._included_opt_parser.get(section, option)
        except (NoOptionError, NoSectionError):
            pass
        else:
            return val

        # Option not found on the included files, now try main file.
        try:
            val = self._main_opt_parser.get(section, option)
        except NoSectionError:
            raise GadgetConfigParserError(
                "No section '{0}'.".format(section))
        except NoOptionError:
            raise GadgetConfigParserError(
                "No option '{0}' in section '{1}'.".format(option, section))
        else:
            return val

    def items(self, section):
        """Return a list of (name, value) pairs for each option in the section.

        :param section: Name of the section (group).
        :type section: str

        :return: All pairs (name, value) in the specified section, one for
                 each option.
        :rtype: list(tuple(name, value))

        :raises GadgetConfigParserError: If the specified section does not
        exist.
        """
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        # First add items from main file
        no_section_main = False
        try:
            main_items = self._main_opt_parser.items(section)
        except NoSectionError:
            no_section_main = True
            main_items = tuple()

        no_section_include = False
        try:
            include_items = self._included_opt_parser.items(section)
        except NoSectionError:
            no_section_include = True
            include_items = tuple()

        # Raise error if the section is not found.
        # Note: section not found is different from empty section.
        if no_section_main and no_section_include:
            raise GadgetConfigParserError("No section '{0}'.".format(section))

        res = OrderedDict(main_items)
        res.update(include_items)
        # List created by comprehension for compatibility with Py2 and Py3.
        return [(k, v) for k, v in res.items()]

    def set(self, section, option, value=None):
        """Set the option for the given section with the specified value.

        If the given section exists then set the given option to the specified
        value, otherwise raise GadgetConfigParserError. If no value is
        provided, the option is transformed into a valueless option.

        Important note: Only options in the main options file and not in the
        included files can be modified.

        :param section: Name of the section (group) for the option to set.
        :type section: str
        :param option: Name of the option to set.
        :type option: str
        :type value: Value to set for the given option and section. By default,
                     None (valueless option added).
        :type value: str or None

        :raises GadgetConfigParserError: If the section does not exist, or
        exists only on an included option file, or the option exist in an
        included option file.
        """
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        # If option exists on an included config file then raise error
        if self._included_opt_parser.has_option(section, option):
            raise GadgetConfigParserError(
                "Option '{0}' of section '{1}' cannot be set since "
                "it belongs to an included option file.".format(option,
                                                                section))
        # If section is only present on included files then raise error
        elif self._included_opt_parser.has_section(section) and not \
                self._main_opt_parser.has_section(section):
            raise GadgetConfigParserError(
                "Section '{0}' cannot be modified since it belongs to an "
                "included option file.".format(section))
        # If option not on included files (nor section only) then try to set it
        else:
            try:
                self._main_opt_parser.set(section, option, value)
                self.modified = True
            except NoSectionError:
                raise GadgetConfigParserError(
                    "No section '{0}'.".format(section))

    def remove_option(self, section, option):
        """Remove the specified option from the specified section.

        If the section does not exist, raise GadgetConfigParserError. If the
        option existed to be removed, return True; otherwise return False.

        Important note: Only options in the main option file and not in the
        included files can be removed.

        :param section: Name of the section (group) for the option to be
                        removed.
        :type section: str
        :param option: Name of the option to remove.
        :type option: str
        :return: True if option existed and was removed else False
        :rtype: bool

        :raises GadgetConfigParserError: If the section does not exist or the
        option exists in an included option file.
        """
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        # If option exists on an included config file then raise error
        if self._included_opt_parser.has_option(section, option):
            raise GadgetConfigParserError(
                "Option '{0}' of section '{1}' cannot be removed since "
                "it belongs to an included option file.".format(option,
                                                                section))
        # If option not on included files then try to remove it
        else:
            try:
                res = self._main_opt_parser.remove_option(section, option)
                if res:
                    self.modified = True
                return res
            except NoSectionError:
                raise GadgetConfigParserError(
                    "No section '{0}'.".format(section))

    def remove_section(self, section):
        """Remove the specified section from the configuration.

        If the section in fact existed, return True. Otherwise return False.

        Important note: Only sections in the main option file and not in the
        included files can be removed.

        :param section: Name of the section (group) for the option to be
                        removed.
        :type section: str
        :return: True if section existed as was removed otherwise False
        :rtype: bool

        :raises GadgetConfigParserError: If the section exists in an included
        option file.
        """
        # Sections are case insensitive (convert to lower case).
        section = section.lower()
        # If option exists on an included config file then raise error
        if self._included_opt_parser.has_section(section):
            raise GadgetConfigParserError(
                "Section '{0}' cannot be removed since it belongs to an "
                "included option file.".format(section))
        # If option not on included files (nor section only) then try to set it
        else:
            res = self._main_opt_parser.remove_section(section)
            if res:
                self.modified = True
            return res

    # pylint: disable=W0212
    def write(self, backup_file_path=None):
        """Write configurations to the option file.

        If configurations were modified it saves those changes to file,
        the order of the sections/options is preserved as well as existing
        comments (except inline comments of deleted options or sections).

        :param backup_file_path: if provided, we try to create a create a
                                 backup of the original configuration file on
                                 the provided path. It must be an absolute
                                 path.
        :raises GadgetConfigParserError: If the user does not have permissions
        to overwrite the option file or to create a backup file (if asked to)
        or if an error occurred while parsing the configuration file.
        """
        in_memory_sections = set(self._main_opt_parser.sections())
        read_sections = set()
        read_options = set()
        drop_section = False
        cursect = None
        line = ""

        def optval_tostr(opt, val):
            """Auxiliary function used to obtain a formatted string with
             an option and option value.
             """
            if val:
                return "{0} = {1}".format(opt, val)
            return opt

        if not self.modified:
            # if we did not do any modifications to what was read from file,
            # we can simply exit.
            return
        try:
            with open(self.filename, 'r') as f:
                lines = f.readlines()
        except IOError as err:
            raise GadgetConfigParserError(
                "Option file '{0}' is not readable: "
                "'{1}'".format(self.filename, str(err)),
                cause=err)
        # Create a backup file if provided
        if backup_file_path:
            if not os.path.isabs(backup_file_path):
                raise GadgetConfigParserError(
                    "'{0}' is not an absolute path. Please provide an "
                    "absolute path to the backup file")
            else:
                orig_perms = stat.S_IMODE(os.stat(self.filename).st_mode)
                # ensure that permissions are at most 640 but respect
                # original ones if they are tighter
                backup_perms = orig_perms & (
                stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP)
                # set flag to create file in write only mode
                flags = os.O_WRONLY | os.O_CREAT | os.O_TRUNC
                try:
                    fd = os.open(backup_file_path, flags, backup_perms)
                    with os.fdopen(fd, 'w') as bf:
                        bf.writelines(lines)
                except Exception as err:
                    raise GadgetConfigParserError(
                        "Cannot create backup file '{0}': "
                        "'{1}'".format(backup_file_path, str(err)), cause=err)

        f = None
        try:
            f = open(self.filename, 'w')
            for line in lines:
                # empty lines or comment lines are returned unmodified
                if line.strip() == '' or line.strip().startswith("#") or \
                        line.strip().startswith(";"):
                    f.write(line)
                else:
                    # is it a section header?
                    mo = self._main_opt_parser.SECTCRE.match(line)
                    if mo:
                        if cursect is None:  # First section being read
                            cursect = mo.group('header').lower()
                            # Flag section to be dropped if it is not in the
                            # ConfigParser object
                            drop_section = cursect not in in_memory_sections
                            read_sections.add(cursect)
                        else:
                            # we are going into another section, add any
                            # missing options to the previous section unless
                            # it was removed
                            if not drop_section:
                                parser_items = self._main_opt_parser.items(
                                    cursect)
                                written_new_options = False
                                for opt, val in parser_items:
                                    if opt not in read_options:
                                        written_new_options = True
                                        f.write("{0}\n".format(
                                            optval_tostr(opt, val)))
                                if written_new_options:
                                    # add a new line to the end of the new
                                    # options
                                    f.write("")
                            # get the name of the new section
                            cursect = mo.group('header').lower()
                            # Flag section to be dropped if it is not in the
                            # ConfigParser object
                            drop_section = cursect not in in_memory_sections
                            read_sections.add(cursect)
                            # Reset options read from previous section
                            read_options = set()

                        # write section line if section is not meant to be
                        # removed
                        if not drop_section:
                            f.write(line)
                    # an option line?
                    else:
                        mo = self._main_opt_parser._optcre.match(line)
                        # if an option inside a section
                        if mo and cursect:
                            if drop_section:
                                # if option belongs to a section to be dropped,
                                # remove it
                                continue
                            optname, _, optval = mo.group('option', 'vi',
                                                          'value')
                            x_optname = self._main_opt_parser.optionxform(
                                optname.rstrip())
                            read_options.add(x_optname)
                            # replace option value if it needs replacing
                            try:
                                new_value = self._main_opt_parser.get(
                                    cursect, x_optname)
                            except NoOptionError:
                                # option was removed. Remove it from file
                                continue
                            if new_value == optval:
                                # if value remains the same, leave line as is
                                f.write(line)
                                continue
                            new_str = optval_tostr(optname, new_value)

                            if not optval:
                                old_str = optname
                            else:
                                # calculate index where option name ends
                                options_ends_at = line.find(optname) + \
                                    len(optname)
                                # calculate index where value ends
                                matched_value_index = line.find(
                                    optval, options_ends_at)
                                replace_until_index = matched_value_index + \
                                    len(optval)
                                # we need to replace the old line up until
                                # the value (if exists)
                                old_str = line[:replace_until_index]
                            f.write(line.replace(old_str, new_str))

                        else:
                            # line format is not valid (neither a section nor
                            # an option nor a comment line)
                            raise GadgetConfigParserError(
                                "Write operation failed.  File '{0}' could "
                                "not be parsed correctly, parsing error at "
                                "line '{1}'.".format(self.filename, line))
            # add missing options for last section
            if not drop_section:
                parser_items = self._main_opt_parser.items(cursect)
                written_new_options = False
                for opt, val in parser_items:
                    if opt not in read_options:
                        # if last line read doesn't have a '\n' at the end,
                        # add it manually.
                        if not line.endswith("\n") and not written_new_options:
                            f.write("\n")
                        written_new_options = True
                        f.write("{0}\n".format(optval_tostr(opt, val)))
                if written_new_options:
                    # add a new line in case any new options were added to the
                    # last section
                    f.write("")

            # add new sections and respective options
            parser_sections = self._main_opt_parser.sections()
            for s in parser_sections:
                if s not in read_sections:
                    # if this section was not yet read in the file, it is
                    # a new section. Write it along with its options.
                    f.write("[{0}]\n".format(s))
                    for opt, val in self._main_opt_parser.items(s):
                        f.write("{0}\n".format(optval_tostr(opt, val)))
        except IOError as err:
            raise GadgetConfigParserError(
                "Option file '{0}' is not writable: "
                "'{1}'".format(self.filename, str(err)),
                cause=err)
        else:
            # we've written changes to file, reset modified flag
            self.modified = False
        finally:
            # close the file
            if f:
                f.close()
