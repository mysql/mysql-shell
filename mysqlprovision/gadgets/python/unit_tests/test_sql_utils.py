
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
Tests mysql_gadgets.common.sql_utils
"""

import unittest

from mysql_gadgets.common.sql_utils import (is_quoted_with_backticks,
                                            quote_with_backticks,
                                            quote_with_backticks_definer,
                                            remove_backtick_quoting,)


class TestSQLUtils(unittest.TestCase):
    """Class to test mysql_gadgets.common.sql_utils module.
    """

    def test_is_quoted_with_backticks(self):
        """ Tests is_quoted_with_backticks method"""
        identifier = "`an identifier`"
        self.assertTrue(is_quoted_with_backticks(identifier))
        self.assertTrue(is_quoted_with_backticks(identifier, 'ANSI_QUOTES'))

        identifier = '"an identifier"'
        self.assertTrue(is_quoted_with_backticks(identifier, 'ANSI_QUOTES'))
        self.assertFalse(is_quoted_with_backticks(identifier))

    def test_remove_backtick_quoting(self):
        """ Tests remove_backtick_quoting method"""
        identifier = "``an identifier``"
        self.assertEqual(remove_backtick_quoting(identifier),
                         "`an identifier`")
        identifier = '""an identifier""'
        self.assertEqual(remove_backtick_quoting(identifier, 'ANSI_QUOTES'),
                         '"an identifier"')

    def test_quote_with_backticks(self):
        """ Tests quote_with_backticks method"""
        identifier = "an identifier"
        self.assertEqual(quote_with_backticks(identifier), "`an identifier`")
        identifier = 'an identifier'
        self.assertEqual(quote_with_backticks(identifier, 'ANSI_QUOTES'),
                         '"an identifier"')

    def test_quote_with_backticks_definer(self):
        """ Tests quote_with_backticks_definer method"""
        self.assertEqual(quote_with_backticks_definer(None), None)
        definer = "user"
        self.assertEqual(quote_with_backticks_definer(definer, 'ANSI_QUOTES'),
                         'user')
        definer = "user@localhost"
        self.assertEqual(quote_with_backticks_definer(definer, 'ANSI_QUOTES'),
                         '"user"@"localhost"')


if __name__ == "__main__":
    unittest.main()
