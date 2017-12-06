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
Unit tests for  mysql_gadgets.common.config_parser module.
"""

import os
import shutil
import stat
import sys
import unittest

from mysql_gadgets.common.config_parser import (MySQLOptionsParser,
                                                option_list_to_dictionary)
from mysql_gadgets.exceptions import GadgetConfigParserError
from unit_tests.utils import get_file_differences


class TestConfigParser(unittest.TestCase):
    """TestConfigParser class.
    """

    @classmethod
    def setUpClass(cls):
        """ Setup test class.
        """
        # Set directory with option files for tests.
        cls.option_file_dir = os.path.normpath(
            os.path.join(__file__, "..", "std_data", "option_files"))

    def test_basic(self):
        """Test creation of MySQLOptionsParser and perform basic check.
        """
        work_file = os.path.join(self.option_file_dir, "my_write_results.cnf")
        orig_file = os.path.join(self.option_file_dir, "my.cnf")
        # Create a copy of the original file which we will use to
        # save the changes.
        shutil.copy(orig_file, work_file)
        # Reading a file to which we don't have read permissions should return
        # an error.
        # NOTE: skip this part on windows since chmod does not properly
        # support windows systems.
        if not sys.platform.startswith("win"):
            try:
                current_priv = stat.S_IMODE(os.lstat(work_file).st_mode)
                # remove read privilege from file
                os.chmod(work_file, current_priv & ~stat.S_IRUSR)
                self.assertFalse(os.access(work_file, os.R_OK))
                with self.assertRaises(GadgetConfigParserError) as cm:
                    MySQLOptionsParser(work_file)
                self.assertIn(
                    "Option file '{0}' is not readable".format(work_file),
                    str(cm.exception))
                # Check if root cause of error is correct (IOError).
                self.assertIsNotNone(cm.exception.cause,
                                     "Exception cause cannot be None.")
                self.assertIsInstance(cm.exception.cause, IOError)
            finally:
                os.remove(work_file)

        # Read basic option file.
        option_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))
        # Check file name.
        self.assertEqual(option_file_parser.filename, os.path.join(
            self.option_file_dir, 'my.cnf'))
        # Check sections (case insensitive, i.e. converted to lower case)
        sections = option_file_parser.sections()
        expected_sections = [
            'default', 'client', 'mysqld_safe', 'mysqld', 'delete_section',
            'escape_sequences', 'path_options', 'empty section',
            'delete_section2']
        self.assertListEqual(sections, expected_sections)
        # Check options of specific section.
        options = option_file_parser.options('client')
        expected_options = [
            'password', 'port', 'socket', 'ssl_ca', 'ssl_cert', 'ssl_key',
            'ssl_cipher', 'CaseSensitiveOptions',
            'option_to_delete_with_value', 'option_to_delete_without_value']
        self.assertEqual(options, expected_options)
        # Check specific option value.
        value = option_file_parser.get('client', 'password')
        self.assertEqual(value, '12345')

        # Try to read an invalid (non existing) option file.
        with self.assertRaises(GadgetConfigParserError) as cm:
            MySQLOptionsParser(filename=os.path.join(
                self.option_file_dir, 'not-exist.cnf'))
        self.assertIn(
            "Option file '{0}' is not readable".format(os.path.join(
                self.option_file_dir, 'not-exist.cnf')),
            str(cm.exception))
        # Check if root cause of error is correct (IOError).
        self.assertIsNotNone(cm.exception.cause,
                             "Exception cause cannot be None.")
        self.assertIsInstance(cm.exception.cause, IOError)

        # Try to parse an invalid option file (no sections).
        with self.assertRaises(GadgetConfigParserError) as cm:
            MySQLOptionsParser(filename=os.path.join(
                self.option_file_dir, 'my_error_no_sec.cnf'))
        self.assertIn('could not be parsed', str(cm.exception))
        # Check if root cause of error is not None (MissingSectionHeaderError).
        # Note: Instance type check skipped here to avoid adding a conditional
        # import (for python 2 and 3).
        self.assertIsNotNone(cm.exception.cause,
                             "Exception cause cannot be None.")

        # Try to parse an invalid option file (duplicated sections).
        with self.assertRaises(GadgetConfigParserError) as cm:
            MySQLOptionsParser(filename=os.path.join(
                self.option_file_dir, 'my_error_duplicated_sec.cnf'))
        self.assertIn("File format error: section 'client' is duplicated "
                      "(sections are case insensitive).", str(cm.exception))
        # Check if root cause is None (error not raised internally by Python).
        self.assertIsNone(cm.exception.cause,
                          "Exception cause should be None.")

    def test_include_directives(self):
        """Test reading option files with include directives.
        """
        # Read option file with !include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))
        res_items = opt_file_parser.items('group1')
        expected_items = [('option1', '153'), ('option2', '20')]
        self.assertListEqual(expected_items, res_items)

        # Read option file with !includedir directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_includedir.cnf'))
        res_items = opt_file_parser.items('group4')
        expected_items = [('option3', '200')]
        self.assertListEqual(expected_items, res_items)

        # Read option file with !include and !includedir directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include_all.cnf'))
        res_items = opt_file_parser.items('group1')
        expected_items = [('option1', '15'), ('option2', '20'),
                          ('option3', '0')]
        self.assertListEqual(expected_items, res_items)
        res_items = opt_file_parser.items('group3')
        expected_items = [('option3', '3')]
        self.assertListEqual(expected_items, res_items)

        # Read option file with repeated file in !include.
        with self.assertRaises(GadgetConfigParserError) as cm:
            MySQLOptionsParser(filename=os.path.join(
                self.option_file_dir, 'my_include_error.cnf'))
        self.assertIn('being included again in file', str(cm.exception))
        # Check if root cause is None (error not raised internally by Python).
        self.assertIsNone(cm.exception.cause,
                          "Exception cause should be None.")

        # Read option file with repeated file in !includedir.
        with self.assertRaises(GadgetConfigParserError) as cm:
            MySQLOptionsParser(filename=os.path.join(
                self.option_file_dir, 'my_include_error2.cnf'))
        self.assertIn('being included again in file', str(cm.exception))
        # Check if root cause is None (error not raised internally by Python).
        self.assertIsNone(cm.exception.cause,
                          "Exception cause should be None.")

        # Read option file with invalid file (not exist) in !include.
        with self.assertRaises(GadgetConfigParserError) as cm:
            MySQLOptionsParser(filename=os.path.join(
                self.option_file_dir, 'my_include_error3.cnf'))
        self.assertIn(
            "Option file '{0}' is not readable".format(os.path.join(
                self.option_file_dir, 'my_include_error3.cnf')),
            str(cm.exception))
        # Check if root cause of error is correct (IOError).
        self.assertIsNotNone(cm.exception.cause,
                             "Exception cause cannot be None.")
        self.assertIsInstance(cm.exception.cause, IOError)

        # Read option file with file with errors in !include.
        with self.assertRaises(GadgetConfigParserError) as cm:
            MySQLOptionsParser(filename=os.path.join(
                self.option_file_dir, 'my_include_error4.cnf'))
        self.assertIn('could not be parsed', str(cm.exception))
        # Check if root cause of error is not None (MissingSectionHeaderError).
        # Note: Instance type check skipped here to avoid adding a conditional
        # import (for python 2 and 3).
        self.assertIsNotNone(cm.exception.cause,
                             "Exception cause cannot be None.")

    def test_add_section(self):
        """ Test add_section() method.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))

        self.assertFalse(opt_file_parser.modified)
        # Add new section (case insensitive)
        opt_file_parser.add_section('FoO')
        # adding a new section should set the modified flag.
        self.assertTrue(opt_file_parser.modified)
        # Check sections
        sections = opt_file_parser.sections()
        expected_sections = [
            'default', 'client', 'mysqld_safe', 'mysqld', 'delete_section',
            'escape_sequences', 'path_options', 'empty section',
            'delete_section2', 'foo']
        self.assertListEqual(sections, expected_sections)

        # reset the flag to check that error raising add section operations
        # do not set the flag.
        opt_file_parser.modified = False
        # Try to add already existing section
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.add_section('foo')
        self.assertIn("Section 'foo' already exists.", str(cm.exception))
        self.assertFalse(opt_file_parser.modified)

        # Adding a section with an empty name or None should return an error
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.add_section('')
        self.assertIn("Cannot add an empty section.", str(cm.exception))
        self.assertFalse(opt_file_parser.modified)
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.add_section(None)
        self.assertIn("Cannot add an empty section.", str(cm.exception))
        self.assertFalse(opt_file_parser.modified)

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))

        # test adding a "default" named section, it should work if Python's
        # default section behavior has been overridden. It should not exist
        # and one must be able to add it.
        self.assertFalse(opt_file_parser.has_section('default'))
        opt_file_parser.add_section("default")
        self.assertTrue(opt_file_parser.has_section('default'))
        # a new section was added, so flag must be True again.
        self.assertTrue(opt_file_parser.modified)

    def test_has_section(self):
        """ Test has_section() function.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))

        self.assertFalse(opt_file_parser.modified)
        # Check existing section.
        res = opt_file_parser.has_section('mysqld')
        self.assertTrue(res)

        # Check existing section (case insensitive).
        res = opt_file_parser.has_section('MySQLd')
        self.assertTrue(res)
        res = opt_file_parser.has_section('client')
        self.assertTrue(res)

        # Check not existing section.
        res = opt_file_parser.has_section('not-exist')
        self.assertFalse(res)
        # reading operations do not change the modified flag value
        self.assertFalse(opt_file_parser.modified)

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))
        self.assertFalse(opt_file_parser.modified)
        # Check section only existing in included file (case insensitive).
        res = opt_file_parser.has_section('group4')
        self.assertTrue(res)
        # Check that default section does not exist by default
        res = opt_file_parser.has_section('default')
        self.assertFalse(res)
        res = opt_file_parser.has_section('GROUP4')
        self.assertTrue(res)
        # reading operations do not change the modified flag value
        self.assertFalse(opt_file_parser.modified)

    def test_options(self):
        """ Test options() function.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))

        # Get all options of a specific section.
        # Note: only sections are case insensitive.
        res = opt_file_parser.options('ClienT')
        expected_options = [
            'password', 'port', 'socket', 'ssl_ca', 'ssl_cert', 'ssl_key',
            'ssl_cipher', 'CaseSensitiveOptions',
            'option_to_delete_with_value', 'option_to_delete_without_value'
        ]
        self.assertListEqual(res, expected_options)

        res = opt_file_parser.options('mysqld')
        # test section where there is repeated option with '-' and with '_'.
        expected_options = [
            'option_to_delete_with_value', 'option_to_delete_without_value',
            'master_info_repository', 'user', 'pid_file', 'socket', 'port',
            'basedir', 'datadir', 'tmpdir', 'to_override',
            'to_override_with_value', 'no_comment_no_value', 'lc_messages_dir',
            'skip_external_locking', 'binlog', 'multivalue', 'semi_colon',
            'bind_address', 'log_error']
        self.assertListEqual(res, expected_options)

        # Get options from an empty section.
        res = opt_file_parser.options('empty section')
        self.assertListEqual(res, [])

        # Try to get options for not existing section.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.options('not-exist')
        self.assertIn("No section 'not-exist'.", str(cm.exception))

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))

        # Check that options in include files are also returned.
        res = opt_file_parser.options('Group2')
        expected_options = [
            'option11', 'option1', 'option2', 'option22'
        ]
        self.assertListEqual(res, expected_options)

    def test_has_option(self):
        """ Test has_option() function.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))

        # Check existing option.
        res = opt_file_parser.has_option('mysqld', 'port')
        self.assertTrue(res)

        # Check existing option (section is case insensitive).
        res = opt_file_parser.has_option('MySQLd', 'port')
        self.assertTrue(res)
        res = opt_file_parser.has_option('client', 'CaseSensitiveOptions')
        self.assertTrue(res)
        # using '-' or '_' should make no difference
        res = opt_file_parser.has_option('MySQLd', 'master-info-repository')
        self.assertTrue(res)
        res = opt_file_parser.has_option('MySQLd', 'master_info_repository')
        self.assertTrue(res)
        res = opt_file_parser.has_option('MySQLd', 'master_info-repository')
        self.assertTrue(res)
        # Check not existing option.
        res = opt_file_parser.has_option('mysqld', 'not-exist')
        self.assertFalse(res)
        res = opt_file_parser.has_option('not-exist', 'port')
        self.assertFalse(res)

        # Check that default section doesn't have ConfigParsers behaviour of
        # looking for values on section called default if it is not on the
        # specified secion
        res = opt_file_parser.has_option("mysqld_safe", "password")
        self.assertFalse(res)

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))

        # Check option only existing in included file.
        res = opt_file_parser.has_option('group4', 'option3')
        self.assertTrue(res)
        res = opt_file_parser.has_option('GROUP4', 'option3')
        self.assertTrue(res)

    def test_get(self):
        """ Test get() function.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))

        # Get some values (section is case insensitive).
        res = opt_file_parser.get('mysqld', 'port')
        self.assertEqual('1001', res)
        res = opt_file_parser.get('MySQLd', 'port')
        self.assertEqual('1001', res)
        res = opt_file_parser.get('client', 'CaseSensitiveOptions')
        self.assertEqual('Yes', res)

        # Read options with quotes
        res = opt_file_parser.get('mysqld_safe', 'valid_value_1')
        self.assertEqual("'include comment ( #) symbol'", res)
        res = opt_file_parser.get('mysqld_safe', 'valid_value_2')
        self.assertEqual('"include comment ( #) symbol"', res)

        # Read option that appears twice, once with '_' other with '-'. Last
        # value should be the one returned.
        res = opt_file_parser.get('mysqld', 'master-info-repository')
        self.assertEqual('FILE', res)
        res = opt_file_parser.get('mysqld', 'master_info_repository')
        self.assertEqual('FILE', res)

        # Check that python's ConfigParser default section behavior does not
        # manifest: if section does't have value return value from default
        # section.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('DEFAULT', 'password2')
        self.assertIn("No option 'password2' in section 'default'.",
                      str(cm.exception))
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('mysqld', 'password')
        self.assertIn("No option 'password' in section 'mysqld'.",
                      str(cm.exception))

        # Get value with inline comment.
        res = opt_file_parser.get('mysqld', 'user')
        self.assertEqual('mysql', res)
        res = opt_file_parser.get('delete_section',
                                  'option_to_drop_with_value2')
        self.assertEqual('"value"', res)

        # Try to get a not existing option.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('mysqld', 'not-exist')
        self.assertIn("No option 'not-exist' in section 'mysqld'.",
                      str(cm.exception))

        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('mysqld', 'password')
        self.assertIn("No option 'password' in section 'mysqld'.",
                      str(cm.exception))

        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('not-exist', 'port')
        self.assertIn("No section 'not-exist'.", str(cm.exception))

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))

        # Get option only existing in included file.
        res = opt_file_parser.get('group4', 'option3')
        self.assertEqual('200', res)
        res = opt_file_parser.get('GROUP4', 'option3')
        self.assertEqual('200', res)

        # Check precedence of options considering included files.
        res = opt_file_parser.get('group2', 'option11')
        self.assertEqual('11', res)
        res = opt_file_parser.get('group2', 'option1')
        self.assertEqual('203', res)
        res = opt_file_parser.get('group2', 'option22')
        self.assertEqual('22', res)

        # Check that a section named default is not special, and returns the
        # same non existing section error
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('default', 'non_existing_value')
        self.assertIn("No section 'default'.", str(cm.exception))

    def test_items(self):
        """ Test items() function.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))

        # Get all items of a specific section.
        # Note: only sections are case insensitive.
        res = opt_file_parser.items('ClienT')
        expected_options = [
            ('password', '12345'), ('port', '1000'),
            ('socket', '/var/run/mysqld/mysqld.sock'),
            ('ssl_ca', 'dummyCA'), ('ssl_cert', 'dummyCert'),
            ('ssl_key', 'dummyKey'),
            ('ssl_cipher', 'AES256-SHA:CAMELLIA256-SHA'),
            ('CaseSensitiveOptions', 'Yes'),
            ('option_to_delete_with_value', '20'),
            ('option_to_delete_without_value', None)
        ]
        self.assertListEqual(expected_options, res)

        # check that only the last repeated option value appears within a
        # section
        res = opt_file_parser.items('mysqlD')
        expected_options = [
            ("option_to_delete_with_value", "20"),
            ("option_to_delete_without_value", None),
            ("master_info_repository", "FILE"),
            ("user", "mysql"), ("pid_file", "/var/run/mysqld/mysqld.pid"),
            ("socket", "/var/run/mysqld/mysqld2.sock"), ("port", "1001"),
            ("basedir", "/usr"), ("datadir", "/var/lib/mysql"),
            ("tmpdir", "/tmp"), ("to_override", None),
            ("to_override_with_value", "old_val"),
            ("no_comment_no_value", None),
            ("lc_messages_dir", "/usr/share/mysql"),
            ("skip_external_locking", None), ("binlog", "True"),
            ("multivalue", "Noooooooooooooooo"),
            ("semi_colon", ";"), ("bind_address", "127.0.0.1"),
            ("log_error", "/var/log/mysql/error.log")
        ]
        self.assertEqual(expected_options, res)
        # Get items from an empty section.
        res = opt_file_parser.items('empty section')
        self.assertListEqual([], res)

        # Try to get items for not existing section.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.items('not-exist')
        self.assertIn("No section 'not-exist'.", str(cm.exception))

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))

        # Check that items in include files are returned with correct
        # precedences.
        res = opt_file_parser.items('Group2')
        expected_options = [
            ('option11', '11'), ('option1', '203'), ('option2', '303'),
            ('option22', '22')
        ]
        self.assertListEqual(expected_options, res)

    def test_set(self):
        """ Test set() method.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))

        # Check values before setting them.
        res = opt_file_parser.get('mysqld', 'port')
        self.assertEqual('1001', res)
        res = opt_file_parser.get('client', 'CaseSensitiveOptions')
        self.assertEqual('Yes', res)
        res = opt_file_parser.get('mysqld', 'user')  # with inline comment
        self.assertEqual('mysql', res)
        res = opt_file_parser.get('mysqld', 'master-info-repository')
        self.assertEqual('FILE', res)
        # We've only just read things, so modified flag shouldn't be set
        self.assertFalse(opt_file_parser.modified)
        # Set some values (section is case insensitive).
        opt_file_parser.set('mysqld', 'port', '1111')
        # After successfull modification done, modified flag must be set
        self.assertTrue(opt_file_parser.modified)
        opt_file_parser.set('client', 'CaseSensitiveOptions', 'Oh! Yes!')
        opt_file_parser.set('MySQLd', 'user', 'root')
        opt_file_parser.set('mysqld', 'master-info-repository')
        # Check values after setting them.
        res = opt_file_parser.get('mysqld', 'port')
        self.assertEqual('1111', res)
        res = opt_file_parser.get('client', 'CaseSensitiveOptions')
        self.assertEqual('Oh! Yes!', res)
        res = opt_file_parser.get('mysqld', 'user')
        self.assertEqual('root', res)
        res = opt_file_parser.get('mysqld', 'master-info-repository')
        self.assertIsNone(res)

        # Set a value for not existing option, but existing section (succeed).
        res = opt_file_parser.has_option('mysqld', 'not-exist')
        self.assertFalse(res)
        opt_file_parser.set('mysqld', 'not-exist', 'ok')
        res = opt_file_parser.get('mysqld', 'not-exist')
        self.assertEqual('ok', res)

        # Set option with no value (existing and new).
        res = opt_file_parser.get('mysqld', 'binlog')
        self.assertEqual('True', res)
        res = opt_file_parser.has_option('mysqld', 'no-value-option')
        self.assertFalse(res)
        opt_file_parser.set('mysqld', 'binlog')
        opt_file_parser.set('mysqld', 'no-value-option')
        res = opt_file_parser.get('mysqld', 'binlog')
        self.assertIsNone(res)
        res = opt_file_parser.get('mysqld', 'no-value-option')
        self.assertIsNone(res)

        # Try to set value for not existing section (fail).
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.set('not-exist', 'port', 'fail')
        self.assertIn("No section 'not-exist'.", str(cm.exception))

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))

        # Try to set an option that only exists in an included file.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.set('Group4', 'option3')
        self.assertIn(
            "Option 'option3' of section 'group4' cannot be set since "
            "it belongs to an included option file.", str(cm.exception))

        # Try to set an option that exists in an included file and main file.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.set('GROUP2', 'option1')
        self.assertIn(
            "Option 'option1' of section 'group2' cannot be set since "
            "it belongs to an included option file.", str(cm.exception))

        # Try to set an new option for a section that exists in an included
        # file, but not in the main file.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.set('Group4', 'option')
        self.assertIn(
            "Section 'group4' cannot be modified since it belongs to an "
            "included option file.", str(cm.exception))

        # Set an option that belongs to the main file but not in the included
        # files.
        res = opt_file_parser.get('group2', 'option11')
        self.assertEqual('11', res)
        opt_file_parser.set('group2', 'option11', '1111')
        res = opt_file_parser.get('group2', 'option11')
        self.assertEqual('1111', res)

        # Set an new option for a section that exist in the main and include
        # files.
        res = opt_file_parser.has_option('group2', 'new-option')
        self.assertFalse(res)
        opt_file_parser.set('group2', 'new-option', 'NEW')
        res = opt_file_parser.get('group2', 'new-option')
        self.assertEqual('NEW', res)

    def test_remove_option(self):
        """ Test remove_option() method.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))
        self.assertFalse(opt_file_parser.modified)
        self.assertFalse(opt_file_parser.has_section('not-exist'))
        # Try to remove value for not existing section (fail).
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.remove_option('not-exist', 'port')
        self.assertIn("No section 'not-exist'.", str(cm.exception))

        self.assertTrue(opt_file_parser.has_section('client'))
        self.assertFalse(opt_file_parser.has_option('client',
                                                    'does_not_exist'))
        # Try to remove value for existing section but non existing option
        res = opt_file_parser.remove_option('client', 'does_not_exist')
        self.assertFalse(res)

        # Check values before removing them them.
        res = opt_file_parser.get('mysqld', 'port')
        self.assertEqual('1001', res)
        res = opt_file_parser.get('client', 'CaseSensitiveOptions')
        self.assertEqual('Yes', res)
        res = opt_file_parser.get('mysqld', 'user')  # with inline comment
        self.assertEqual('mysql', res)
        self.assertFalse(opt_file_parser.modified)

        # Remove some values (section is case insensitive).
        res = opt_file_parser.remove_option('mysqld', 'port')
        self.assertTrue(res, True)
        self.assertTrue(opt_file_parser.modified)
        res = opt_file_parser.remove_option('client', 'CaseSensitiveOptions')
        self.assertTrue(res, True)
        res = opt_file_parser.remove_option('MySQLd', 'user')
        self.assertTrue(res, True)
        self.assertTrue(opt_file_parser.modified)
        # Check values after removing them.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('mysqld', 'port')
        self.assertIn("No option 'port' in section 'mysqld'",
                      str(cm.exception))
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('client', 'CaseSensitiveOptions')
        self.assertIn("No option 'CaseSensitiveOptions' in section 'client'",
                      str(cm.exception))
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.get('mysqld', 'user')
        self.assertIn("No option 'user' in section 'mysqld'",
                      str(cm.exception))

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))
        self.assertFalse(opt_file_parser.modified)

        # Try to remove an option that only exists in an included file.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.remove_option('Group4', 'option3')
        self.assertIn(
            "Option 'option3' of section 'group4' cannot be removed since "
            "it belongs to an included option file.", str(cm.exception))

        # Try to remove an option that exists in an included file and main
        # file.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.remove_option('GROUP2', 'option1')
        self.assertIn(
            "Option 'option1' of section 'group2' cannot be removed since "
            "it belongs to an included option file.", str(cm.exception))
        self.assertFalse(opt_file_parser.modified)

        # Try to remove an option for a section that exists in an included
        # file and in the main file but whose option only exists in the main
        #  file.
        res = opt_file_parser.remove_option('group2', 'option11')
        self.assertTrue(res)
        self.assertTrue(opt_file_parser.modified)

    def test_remove_section(self):
        """ Test remove_section() method.
        """
        # Read base option file.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my.cnf'))

        # Check values before setting them.
        res = opt_file_parser.has_section('mysqld')
        self.assertTrue(res)
        res = opt_file_parser.has_section('client')
        self.assertTrue(res)
        res = opt_file_parser.has_section('default')
        self.assertTrue(res)

        # Parser hasn't been modified, so modified flag shouldn't be set
        self.assertFalse(opt_file_parser.modified)
        # Try to remove non existing section should return False.
        res = opt_file_parser.remove_section('not-exist')
        self.assertFalse(res)
        # since we didn't remove anything, flag should still be False
        self.assertFalse(opt_file_parser.modified)

        # Remove some sections (section is case insensitive). If they exist
        # it should return True.
        res = opt_file_parser.remove_section('mysqld')
        self.assertTrue(res, True)
        # since we removed a section, modfified flag must have been set to True
        self.assertTrue(opt_file_parser.modified)
        res = opt_file_parser.remove_section('client')
        self.assertTrue(res, True)
        res = opt_file_parser.remove_section('default')
        self.assertTrue(res, True)
        # Check sections after removing them.
        res = opt_file_parser.has_section('mysqld')
        self.assertFalse(res)
        res = opt_file_parser.has_section('client')
        self.assertFalse(res)
        res = opt_file_parser.has_section('default')
        self.assertFalse(res)

        # Read option file with include directives.
        opt_file_parser = MySQLOptionsParser(filename=os.path.join(
            self.option_file_dir, 'my_include.cnf'))
        self.assertFalse(opt_file_parser.modified)
        # Try to remove an section that only exists in an included file.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.remove_section('client')
        self.assertIn(
            "Section 'client' cannot be removed since it belongs to an "
            "included option file.", str(cm.exception))

        # Try to remove a section that exists in an included file and main
        # file.
        with self.assertRaises(GadgetConfigParserError) as cm:
            opt_file_parser.remove_section('GROUP2')
        self.assertIn(
            "Section 'group2' cannot be removed since it belongs to an "
            "included option file.", str(cm.exception))
        self.assertFalse(opt_file_parser.modified)

        # Try to remove a section that only exists in the main
        # file.
        res = opt_file_parser.remove_section('mysql')
        self.assertTrue(res)
        self.assertTrue(opt_file_parser.modified)
        res = opt_file_parser.has_section('mysql')
        self.assertFalse(res)
        self.assertTrue(opt_file_parser.modified)

    # pylint: disable=R0915
    def test_write(self):
        """ Test write() method.
        """
        work_file = os.path.join(self.option_file_dir, "my_write_results.cnf")
        orig_file = os.path.join(self.option_file_dir, "my.cnf")
        expected_results_file = os.path.join(self.option_file_dir,
                                             "my_expected_results.cnf")
        # Create a copy of the original file which we will use to
        # save the changes.
        shutil.copy(orig_file, work_file)
        backup_file = os.path.join(self.option_file_dir, "backup_file.cnf")

        try:
            opt_file_parser = MySQLOptionsParser(work_file)
            # do some read_only operations
            opt_file_parser.sections()
            opt_file_parser.get('default', 'password')
            # when no changes are made, write() returns immediately, since it
            # doesn't need to save anything. We provide it with an invalid
            # path to a backup file and check that it doesn't return any error
            self.assertFalse(opt_file_parser.modified)
            opt_file_parser.write(backup_file_path="../not_valid_path")
            self.assertFalse(opt_file_parser.modified)
            # Check that work file is equal to original file
            diff = get_file_differences(orig_file, work_file)
            if diff:
                self.fail(diff)

            # modify the file and check the modified flag has been set
            opt_file_parser.add_section("new_section")
            self.assertTrue(opt_file_parser.modified)

            # NOTE: skip this part on windows since chmod does not properly
            # support windows systems.
            # if the configuration file's read permissions have been removed,
            # the write method should raise an error.
            if not sys.platform.startswith("win"):
                orig_priv = stat.S_IMODE(os.lstat(work_file).st_mode)
                # remove read privilege
                os.chmod(work_file, orig_priv & ~stat.S_IRUSR)
                self.assertFalse(os.access(work_file, os.R_OK))
                with self.assertRaises(GadgetConfigParserError) as cm:
                    opt_file_parser.write()
                self.assertIn(
                    "Option file '{0}' is not readable"
                    "".format(work_file), str(cm.exception))
                # Check if root cause of error is correct (IOError).
                self.assertIsNotNone(cm.exception.cause,
                                     "Exception cause cannot be None.")
                self.assertIsInstance(cm.exception.cause, IOError)
                # since the write wasn't successful the modified flag must
                # still be set to True
                self.assertTrue(opt_file_parser.modified)
                # Give read permission back again and remove write permissions
                os.chmod(work_file, orig_priv | stat.S_IRUSR)
                os.chmod(work_file, orig_priv & ~stat.S_IWUSR)
                self.assertTrue(os.access(work_file, os.R_OK))
                self.assertFalse(os.access(work_file, os.W_OK))
                # trying to save modified configurations to a config file where
                # we don't have write permissions should also return an error
                with self.assertRaises(GadgetConfigParserError) as cm:
                    opt_file_parser.write()
                self.assertIn(
                    "Option file '{0}' is not writable"
                    "".format(work_file), str(cm.exception))
                # Check if root cause of error is correct (IOError).
                self.assertIsNotNone(cm.exception.cause,
                                     "Exception cause cannot be None.")
                self.assertIsInstance(cm.exception.cause, IOError)
                # since the write wasn't successful the modified flag must
                # still be set to True
                self.assertTrue(opt_file_parser.modified)
                # Give write permission again
                os.chmod(work_file, orig_priv | stat.S_IWUSR)

                # if a backup file exists but is not writable
                shutil.copy(work_file, backup_file)
                backup_priv = stat.S_IMODE(os.lstat(backup_file).st_mode)
                os.chmod(backup_file, backup_priv & ~stat.S_IWUSR)
                self.assertFalse(os.access(backup_file, os.W_OK))
                with self.assertRaises(GadgetConfigParserError) as cm:
                    opt_file_parser.write(backup_file_path=backup_file)
                self.assertIn(
                    "Backup file '{0}' is not writable"
                    "".format(backup_file), str(cm.exception))
                # Check if root cause of error is correct (IOError).
                self.assertIsNotNone(cm.exception.cause,
                                     "Exception cause cannot be None.")
                self.assertIsInstance(cm.exception.cause, IOError)
                # since the write wasn't successful the modified flag must
                # still be set to True
                self.assertTrue(opt_file_parser.modified)
                # remove backup_file
                os.remove(backup_file)
            # if the config file has been modified after being read and its
            # format is invalid, an exception must be raised
            with open(work_file, 'a') as f:
                # introduce an invalid line
                f.write("\n=no_option_only_value")
            with self.assertRaises(GadgetConfigParserError) as cm:
                opt_file_parser.write()
            self.assertIn(
                "Write operation failed.  File '{0}' could not be parsed "
                "correctly, parsing error at line '=no_option_only_value'."
                "".format(work_file), str(cm.exception))
            # Check if root cause is None (error not raised internally by
            # Python).
            self.assertIsNone(cm.exception.cause,
                              "Exception cause should be None.")
            # Remove work file and create a new valid copy.
            os.remove(work_file)
            shutil.copy(orig_file, work_file)
            # Add a new options with and without values
            opt_file_parser.set("new_section", "new_option", "new_value")
            opt_file_parser.set("new_section", "option_no_value")

            # override existing option with comment and value
            opt_file_parser.set("mysqld", "to_override_with_value",
                                "new_value")
            # override existing option with comment and no value
            opt_file_parser.set("mysqld", "to_override", "overridden")
            # override existing option with comment and no value
            opt_file_parser.set("mysqld", "log_error")
            # override existing option without comment and without value
            opt_file_parser.set("mysqld", "no_comment_no_value",
                                "'look a value #'")

            # add new option to existing section
            opt_file_parser.set("mysqld", "new_option_existing_section",
                                "new_value")
            opt_file_parser.set("mysqld",
                                "new_option_existing_section_no_value")

            # modify option with # inside quotes
            opt_file_parser.set("mysqld_safe", "valid_value_2",
                                '"modified include # symbol"')
            # modify repeated option
            opt_file_parser.set("mysqld", "master_info-repository",
                                "'TABLE' #This value has been changed")

            # Remove options
            opt_file_parser.remove_option("client",
                                          "option_to_delete_with_value")
            opt_file_parser.remove_option("client",
                                          "option_to_delete_without_value")

            opt_file_parser.remove_option("mysqld",
                                          "option_to_delete_with_value")
            opt_file_parser.remove_option("mysqld",
                                          "option_to_delete_without_value")
            opt_file_parser.remove_option("default", "repeated_value")

            # Remove sections
            opt_file_parser.remove_section("delete_section2")
            opt_file_parser.remove_section("delete_section")
            opt_file_parser.remove_section("delete_section0")

            # writing to file, resets the modified flag
            self.assertTrue(opt_file_parser.modified)

            # passing a non absolute path must return an error
            with self.assertRaises(GadgetConfigParserError) as cm:
                opt_file_parser.write("../not_an_absolute_path")
            # Check if root cause is None (error not raised internally by
            # Python).
            self.assertIsNone(cm.exception.cause,
                              "Exception cause should be None.")
            # passing an invalid path to backup must also return an error
            with self.assertRaises(GadgetConfigParserError) as cm:
                opt_file_parser.write("/not/a_valid/path")
            # Check if root cause of error is correct (IOError).
            self.assertIsNotNone(cm.exception.cause,
                                 "Exception cause cannot be None.")
            self.assertIsInstance(cm.exception.cause, IOError)

            opt_file_parser.write(backup_file)
            # since the write was successful the modified flag must set to
            # False
            self.assertFalse(opt_file_parser.modified)

            # Check that backup file is equal to original file
            diff = get_file_differences(orig_file, backup_file)
            # fail if diff is not empty
            if diff:
                self.fail(diff)

            # check that written file is equal to expected file
            diff = get_file_differences(work_file, expected_results_file)
            # fail if diff is not empty
            if diff:
                self.fail(diff)
        finally:
            try:
                os.remove(work_file)
            except OSError:
                pass
            try:
                os.remove(backup_file)
            except OSError:
                pass

    def test_write_on_last_section(self):
        """test writing an option on the last section of the file"""

        work_file = os.path.join(self.option_file_dir, "my_write_results.cnf")
        orig_file = os.path.join(self.option_file_dir, "my.cnf")
        # test setting option in the last section of the file.
        shutil.copy(orig_file, work_file)
        try:
            opt_file_parser = MySQLOptionsParser(work_file)
            self.assertTrue(opt_file_parser.has_section('delete_section2'))
            # make sure that delete_section2 is the last section
            self.assertTrue(opt_file_parser.sections()[-1], 'delete_section2')
            # New option in pre-existing section delete_section2
            opt_file_parser.set('delete_section2', 'relay-log-info-repository',
                                "a,long,list,of,values")
            self.assertTrue(opt_file_parser.modified)
            # write the changes
            opt_file_parser.write()
            # now we read the new option added to the last section
            opt_file_parser = MySQLOptionsParser(work_file)
            self.assertTrue(opt_file_parser.has_option(
                'delete_section2', 'relay-log-info-repository'))
            self.assertEqual(
                opt_file_parser.get('delete_section2',
                                    'relay-log-info-repository'),
                "a,long,list,of,values")
            # remove new option
            opt_file_parser.remove_option('delete_section2',
                                          'relay-log-info-repository')
            # Write the file
            opt_file_parser.write()
            # verify the option 'relay-log-info-repository' has gone
            opt_file_parser = MySQLOptionsParser(work_file)
            # verify option relay-log-info-repository does not exist
            self.assertTrue(opt_file_parser.has_section(
                'delete_section2'))
            self.assertFalse(opt_file_parser.has_option(
                'delete_section2', 'relay-log-info-repository'))
        finally:
            try:
                os.remove(work_file)
            except OSError:
                pass

    def test_write_option_whose_value_is_substring(self):
        """Test writing an option whose value is a substring of the name of
        the option"""
        work_file = os.path.join(self.option_file_dir, "my_write_results.cnf")
        orig_file = os.path.join(self.option_file_dir, "my.cnf")
        # test set option updating an option whose value is part of the name.
        shutil.copy(orig_file, work_file)
        try:
            opt_file_parser = MySQLOptionsParser(work_file)
            self.assertTrue(opt_file_parser.has_section('mysqld'))
            # add a new option whose value is a substring of the name of the
            # option
            opt_file_parser.set('mysqld', 'write-log-messages-to',
                                "log")
            self.assertTrue(opt_file_parser.modified)
            # write the changes
            opt_file_parser.write()
            # now we read the new option added
            opt_file_parser = MySQLOptionsParser(work_file)
            self.assertTrue(opt_file_parser.has_option(
                'mysqld', 'write-log-messages-to'))
            self.assertEqual(opt_file_parser.get(
                'mysqld', 'write-log-messages-to'), "log")
            # Update write-log-messages-to option value
            opt_file_parser.set('mysqld', 'write-log-messages-to',
                                "stdout")
            # write the changes
            opt_file_parser.write()
            # check updated value
            opt_file_parser = MySQLOptionsParser(work_file)
            self.assertTrue(opt_file_parser.has_section('mysqld'))
            self.assertTrue(opt_file_parser.has_option(
                'mysqld', 'write-log-messages-to'))
            # retrieve the value for later use
            opt_val = opt_file_parser.get('mysqld',
                                          'write-log-messages-to')
            self.assertEqual(opt_val, "stdout")
        finally:
            try:
                os.remove(work_file)
            except OSError:
                pass

    def test_option_list_to_dictionary(self):
        """Test option_list_to_dictionary function"""
        expected_res = {"opt_without_val": None, "opt_with_val": "val",
                        "opt_without_val2": ''}

        res = option_list_to_dictionary(["opt-without_val", "opt_with_val=val",
                                         "opt-without-val2="])
        self.assertEqual(res, expected_res)

if __name__ == "__main__":
    unittest.main()
