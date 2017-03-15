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
This module contains functions to manipulate SQL and database object names.
"""


def quote_with_backticks(identifier, sql_mode=''):
    """Quote the given identifier with backticks, converting backticks (`) in
    the identifier name with the correct escape sequence (``) unless the
    identifier is quoted (") as in sql_mode set to ANSI_QUOTES.

    :param: identifier: Identifier to quote.
    :type identifier:   string
    :param sql_mode:    The sql_mode set on the server.
    :type sql_mode:     string

    :return: String with the identifier quoted with backticks.
    :rtype:  string
    """
    if "ANSI_QUOTES" in sql_mode:
        return '"{0}"'.format(identifier.replace('"', '""'))
    else:
        return "`{0}`".format(identifier.replace("`", "``"))


def quote_with_backticks_definer(definer, sql_mode=''):
    """Quote the given definer clause with backticks.

    This functions quotes the given definer clause with backticks, converting
    backticks (`) in the string with the correct escape sequence (``).

    :param definer:     Definer clause to quote.
    :type definer:      string
    :param sql_mode:    The sql_mode set on the server.
    :type sql_mode:     string

    :return:  String with the definer quoted with backticks.
    :rtype:   string
    """
    if not definer:
        return definer
    parts = definer.split('@')
    if len(parts) != 2:
        return definer
    return '@'.join([quote_with_backticks(parts[0], sql_mode),
                     quote_with_backticks(parts[1], sql_mode)])


def remove_backtick_quoting(identifier, sql_mode=''):
    """Remove backtick quoting from the given identifier, reverting the
    escape sequence (``) to a backtick (`) in the identifier name.

    :param identifier:  Identifier to remove backtick quotes.
    :type identifier:   string
    :param sql_mode:    The sql_mode set on the server.
    :type sql_mode:     string

    :return: String with the identifier without backtick quotes.
    :rtype:  string
    """
    double_quoting = identifier.startswith('"') and identifier.endswith('"')
    # remove quotes
    identifier = identifier[1:-1]
    if 'ANSI_QUOTES' in sql_mode and double_quoting:
        return identifier.replace('""', '"')
    else:
        # Revert backtick escape sequence
        return identifier.replace("``", "`")


def is_quoted_with_backticks(identifier, sql_mode=''):
    """Check if the given identifier is quoted with backticks.

    :param identifier:  identifier to check.
    :type  identifier:  string
    :param sql_mode:    The sql_mode set on the server.
    :type sql_mode:     string

    :return: True if the identifier has backtick quotes, and False otherwise.
    :rtype: bool
    """
    if 'ANSI_QUOTES' in sql_mode:
        return (identifier[0] == "`" and identifier[-1] == "`") or \
            (identifier[0] == '"' and identifier[-1] == '"')
    else:
        return identifier[0] == "`" and identifier[-1] == "`"
