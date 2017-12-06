#
# Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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
"""
This module contains helper methods for formatting output.
"""


import os
import struct
# pylint: disable=E0401
if os.name == 'posix':
    import fcntl
    import termios
else:
    from ctypes import windll, create_string_buffer


MIN_COLUMN_SIZE = 25  # min column size for the value

_TWO_COLUMN_DISPLAY = "{0:{1}}  {2:{3}}\n"


def get_terminal_size():
    """Return the number of characters that conforms the terminal (in columns,
    rows for terminal window).

    This method will attempt to determine the current terminal window size.
    If it cannot, it returns the default of (78, 25) characters

    :returns: A tuple of integers (x, y) that represents the maximum (# chars)
              columns and maximum rows
    :rtype: tuple
    """
    default = (78, 25)
    try:
        if os.name == "posix":
            y, x = 0, 1
            packed_info = fcntl.ioctl(0, termios.TIOCGWINSZ,
                                      struct.pack('HHHH', 0, 0, 0, 0))
            wininfo = struct.unpack('HHHH', packed_info)
            return wininfo[x], wininfo[y]
        else:
            strbuff = create_string_buffer(22)
            handle = windll.kernel32.GetStdHandle(-11)
            windll.kernel32.GetConsoleScreenBufferInfo(handle, strbuff)
            left, top, right, bottom = 5, 6, 7, 8
            wininfo = struct.unpack("hhhhHhhhhhh", strbuff)
            x = wininfo[right] - wininfo[left] + 1
            y = wininfo[bottom] - wininfo[top] + 1
            return x, y
    # pylint: disable=W0702
    except:
        pass  # silence! just return default on error.
    return default


def get_max_display_width():
    """Returns the maximum width for the console.

    :returns: width of the console
    :rtype: int
    """
    if isinstance(os.environ.get("COLUMNS", 78), str):
        try:
            _MAX_WIDTH = int(os.environ.get("COLUMNS", 78))
        except ValueError:
            _MAX_WIDTH = get_terminal_size()[0]
    else:
        _MAX_WIDTH = get_terminal_size()[0]
    # When the utility is run by another script the size is 1 but it causes
    # textwrap to fail, return 78
    if _MAX_WIDTH == 1:
        _MAX_WIDTH = 78
    return _MAX_WIDTH
