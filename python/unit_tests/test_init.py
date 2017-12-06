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
Unit tests for  mysql_gadgets.__init__ module.
"""

import unittest

from mysql_gadgets import (check_connector_python, check_python_version,
                           check_expected_version, VERSION)
from mysql_gadgets.exceptions import GadgetError


class TestInit(unittest.TestCase):
    """TestInit class.
    """

    def test_check_connector_python(self):  # pylint: disable=R0201
        """ Test check_connector_python method.
        """
        # Test current Connector/Python version (execution should continue
        # and not exit).
        # Note: Execution stops and exit if version is not supported.
        check_connector_python()

    def test_check_python_version(self):
        """ Test check_python_version method.
        """
        # Test current Python version
        self.assertTrue(check_python_version(),
                        "Current Python version expected to be supported.")

        # Check for non existing Python version and raise exception.
        with self.assertRaises(GadgetError) as test_raises:
            check_python_version(min_version=(99, 0, 0),
                                 max_version=(99, 99, 99), exit_on_fail=False,
                                 raise_exception_on_fail=True)
        self.assertIn("requires Python version 99.0.0 or higher and lower "
                      "than 99.99.99.", test_raises.exception.errmsg)

        # Check for non existing Python version and return error message.
        res, err_msg = check_python_version(min_version=(99, 0, 0),
                                            max_version=(99, 99, 99),
                                            exit_on_fail=False,
                                            return_error_msg=True)
        self.assertFalse(res)
        self.assertIn("requires Python version 99.0.0 or higher and lower "
                      "than 99.99.99.", err_msg)

        # Check for non existing Python version with no max version.
        with self.assertRaises(GadgetError) as test_raises:
            check_python_version(min_version=(99, 9, 9),
                                 max_version=None, exit_on_fail=False,
                                 raise_exception_on_fail=True)
        self.assertIn("requires Python version 99.9.9 or higher.",
                      test_raises.exception.errmsg)
        res, err_msg = check_python_version(min_version=(99, 9, 9),
                                            max_version=None,
                                            exit_on_fail=False,
                                            return_error_msg=True)
        self.assertFalse(res)
        self.assertIn("requires Python version 99.9.9 or higher.", err_msg)

    def test_check_expected_version(self):
        """ Test check_expected_version method.
        """
        # Test invalid parameters used for check_expected_version().
        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version('')
        self.assertIn("Invalid expected version value: ''.",
                      str(err_cm.exception))

        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version(None)
        self.assertIn("Invalid expected version value: 'None'.",
                      str(err_cm.exception))

        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version('1.1.1.1.1.1.1')
        self.assertIn("Invalid expected version value: '1.1.1.1.1.1.1'.",
                      str(err_cm.exception))

        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version('x')
        self.assertIn("Invalid integer for the expected major version number: "
                      "'x'", str(err_cm.exception))

        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version('-1')
        self.assertIn("Invalid integer for the expected major version, "
                      "it cannot be a negative number: '-1'.",
                      str(err_cm.exception))

        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version('1.x')
        self.assertIn("Invalid integer for the expected minor version number: "
                      "'x'", str(err_cm.exception))

        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version('1.-1')
        self.assertIn("Invalid integer for the expected minor version, "
                      "it cannot be a negative number: '-1'.",
                      str(err_cm.exception))

        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version('1.1.x')
        self.assertIn("Invalid integer for the expected patch version number: "
                      "'x'", str(err_cm.exception))

        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version('1.1.-1')
        self.assertIn("Invalid integer for the expected patch version, "
                      "it cannot be a negative number: '-1'.",
                      str(err_cm.exception))

        # Test not compatible expected versions.
        current_version = VERSION
        # Using greater major expected version.
        test_version = str(current_version[0] + 1)
        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version(test_version)
        self.assertIn("The expected version is not compatible with the "
                      "current version.", str(err_cm.exception))
        # Using lower major expected version.
        # Note: can only be tested if current major version number is > 0.
        if current_version[0] > 0:
            test_version = str(current_version[0] - 1)
            with self.assertRaises(GadgetError) as err_cm:
                check_expected_version(test_version)
            self.assertIn("The expected version is not compatible with the "
                          "current version.", str(err_cm.exception))
        # Using greater minor expected version.
        test_version = "{0}.{1}".format(current_version[0],
                                        current_version[1] + 1)
        with self.assertRaises(GadgetError) as err_cm:
            check_expected_version(test_version)
        self.assertIn("The expected version is not compatible with the "
                      "current version.", str(err_cm.exception))

        # Test compatible expected versions.
        test_version = str(current_version[0])
        # Same major version number.
        check_expected_version(test_version)
        # Same major and minor version number.
        test_version = "{0}.{1}".format(current_version[0], current_version[1])
        check_expected_version(test_version)
        # Same major version number and lower expected minor version number.
        # Note: can only be tested if current minor version number is > 0.
        if current_version[1] > 0:
            test_version = "{0}.{1}".format(current_version[0],
                                            current_version[1] - 1)
        check_expected_version(test_version)
        # Same major, minor and patch version number.
        test_version = "{0}.{1}.{2}".format(current_version[0],
                                            current_version[1],
                                            current_version[2])
        check_expected_version(test_version)
        # Same major and minor version numbers and higher patch version number.
        test_version = "{0}.{1}.{2}".format(current_version[0],
                                            current_version[1],
                                            current_version[2] + 1)
        check_expected_version(test_version)
        # Same major and minor version numbers and lower patch version number.
        # Note: can only be tested if current patch version number is > 0.
        if current_version[2] > 0:
            test_version = "{0}.{1}.{2}".format(current_version[0],
                                                current_version[1],
                                                current_version[2] - 1)
            check_expected_version(test_version)

if __name__ == '__main__':
    unittest.main()
