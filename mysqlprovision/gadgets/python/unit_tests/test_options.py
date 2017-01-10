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
Unit tests for mysql_gadgets.common.options module.
"""
import argparse
import os
import sys
import unittest
try:
    # python2
    from StringIO import StringIO
except ImportError:
    # python3
    from io import StringIO

from mysql_gadgets.common import options


class TestParsingOptions(unittest.TestCase):
    """Unit Test Class for the mysql_gadgets.common.options module.
    """
    def setUp(self):
        """setUp method"""
        self.parser = argparse.ArgumentParser("Test Parser")

    def test_add_store_connection_option(self):
        """test add_store_connection option"""
        options.add_store_connection_option(self.parser, ["--source"],
                                            "source")
        # by default it should be required, so not passing it or passing it
        # empty should be an error
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args("")
        self.assertEqual(err.exception.code, 2)
        with self.assertRaises(SystemExit):
            self.parser.parse_args(["--source="])
        self.assertEqual(err.exception.code, 2)

        # Passing it an incorrect value should also be an error
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args(["--source='this_is_not_valid'"])
        self.assertEqual(err.exception.code, 2)

        # Correct values
        # by default you only need to specify user and host, and port is 3306
        parsed = self.parser.parse_args(
            "--source=root@localhost".split())
        self.assertEqual(
            parsed,
            argparse.Namespace(
                source={"host": "localhost", "user": "root", "port": 3306},
                store_password_list=[('source', 'root@localhost')]))

        # but you can also specify a port or a socket
        parsed = self.parser.parse_args(
            "--source=root@localhost:13001".split())
        self.assertEqual(
            parsed,
            argparse.Namespace(
                source={"host": "localhost", "user": "root", "port": 13001},
                store_password_list=[('source', 'root@localhost:13001')]))

        parsed = self.parser.parse_args(
            "--source=root@localhost:path_to_socket".split())

        if os.name == 'nt':
            self.assertEqual(
                parsed,
                argparse.Namespace(
                    source={"host": "localhost", "user": "root", 'port': 3306},
                    store_password_list=[('source',
                                          'root@localhost:path_to_socket')]))
        else:
            self.assertEqual(
                parsed,
                argparse.Namespace(
                    source={"host": "localhost", "user": "root",
                            "unix_socket": "path_to_socket", "port": None},
                    store_password_list=[('source',
                                          'root@localhost:path_to_socket')]))

        # Add a non required store connection option that also doesn't require
        # passwords
        options.add_store_connection_option(self.parser, ["--destination"],
                                            "destination", required=False,
                                            ask_pass=False)

        # Not passing a non required connection must work
        parsed = self.parser.parse_args(
            "--source=root@localhost:13002"
            "".split())
        self.assertEqual(parsed, argparse.Namespace(
            source={"host": "localhost", "user": "root", "port": 13002},
            destination=None,
            store_password_list=[('source', 'root@localhost:13002')]))

        # Since the ask_pass value for the destination option is False, the
        # argument received by the option must not be on the
        # store_password_list
        parsed = self.parser.parse_args(
            "--source=root@localhost:13002 --destination=user1@127.0.0.1:13003"
            "".split())
        self.assertEqual(parsed, argparse.Namespace(
            source={"host": "localhost", "user": "root", "port": 13002},
            destination={"host": "127.0.0.1", "user": "user1", "port": 13003},
            store_password_list=[('source', 'root@localhost:13002')]))

    def test_add_append_connection_option(self):
        """"test add_append_connection_option"""
        options.add_append_connection_option(self.parser, ["--store"], "store")
        # by default it should be required, so not passing it or passing it
        # empty should be an error
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args("")
        self.assertEqual(err.exception.code, 2)
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args(["--store="])
        self.assertEqual(err.exception.code, 2)

        # Passing it an incorrect value should also be an error
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args(["--store='this_is_not_valid'"])
        self.assertEqual(err.exception.code, 2)

        # Correct values
        # by default you only need to specify user and host, and port is 3306
        parsed = self.parser.parse_args("--store=root@localhost".split())
        self.assertEqual(parsed, argparse.Namespace(
            _store_cache=set([(('host', 'localhost'), ('port', 3306),
                               ('user', 'root'))]),
            store=[{"host": "localhost", "user": "root", "port": 3306}],
            append_password_list=[('store', 'root@localhost')]))
        # but you can also specify a port or a socket
        parsed = self.parser.parse_args(
            "--store=root@localhost:13001".split())
        self.assertEqual(parsed, argparse.Namespace(
            _store_cache=set([(('host', 'localhost'), ('port', 13001),
                               ('user', 'root'))]),
            store=[{"host": "localhost", "user": "root", "port": 13001}],
            append_password_list=[('store', 'root@localhost:13001')]))

        parsed = self.parser.parse_args(
            "--store=root@localhost:path_to_socket".split())
        if os.name == 'nt':
            self.assertEqual(parsed, argparse.Namespace(
                _store_cache=set([(('host', 'localhost'), ('port', 3306),
                                   ('user', 'root'))]),
                store=[{"host": "localhost", "user": "root", 'port': 3306}],
                append_password_list=[('store',
                                       'root@localhost:path_to_socket')]))
        else:
            self.assertEqual(parsed, argparse.Namespace(
                _store_cache=set([(('host', 'localhost'),
                                   ('port', None),
                                   ('unix_socket', 'path_to_socket'),
                                   ('user', 'root'))]),
                store=[{"host": "localhost", "user": "root",
                        "unix_socket": "path_to_socket", "port": None}],
                append_password_list=[('store',
                                       'root@localhost:path_to_socket')]))
        # and you can use it several times to pass several connections
        parsed = self.parser.parse_args("--store=root@localhost "
                                        "--store=root@localhost:13002 "
                                        "--store=root@localhost:13003".split())
        self.assertEqual(parsed, argparse.Namespace(
            _store_cache=set([(('host', 'localhost'), ('port', 13002),
                               ('user', 'root')),
                              (('host', 'localhost'), ('port', 13003),
                               ('user', 'root')),
                              (('host', 'localhost'), ('port', 3306),
                               ('user', 'root'))]),
            store=[{"host": "localhost", "user": "root", "port": 3306},
                   {"host": "localhost", "user": "root", "port": 13002},
                   {"host": "localhost", "user": "root", "port": 13003}],
            append_password_list=[('store', 'root@localhost'),
                                  ('store', 'root@localhost:13002'),
                                  ('store', 'root@localhost:13003')]))
        # but you cannot pass the same value more than once
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args("--store=root@localhost "
                                   "--store=root@localhost:3306 "
                                   "--store=root@localhost:13003".split())
        self.assertEqual(err.exception.code, 2)

        # Add a non required append connection option that also doesn't
        # require passwords
        options.add_append_connection_option(self.parser, ["--nstore"],
                                             "nstore", required=False,
                                             ask_pass=False)
        # Not passing a non required connection must work
        parsed = self.parser.parse_args("--store=root@localhost:13001".split())
        self.assertEqual(parsed, argparse.Namespace(
            _store_cache=set([(('host', 'localhost'), ('port', 13001),
                               ('user', 'root'))]),
            store=[{"host": "localhost", "user": "root", "port": 13001}],
            nstore=None,
            append_password_list=[('store', 'root@localhost:13001')]))

        # Since the ask_pass value for the nstore option is False, the
        # argument received by the option must not be on the
        # store_password_list
        parsed = self.parser.parse_args("--store=root@localhost:13001 "
                                        "--nstore=root@localhost:13001"
                                        "".split())
        self.assertEqual(parsed, argparse.Namespace(
            _store_cache=set([(('host', 'localhost'), ('port', 13001),
                               ('user', 'root'))]),
            store=[{"host": "localhost", "user": "root", "port": 13001}],
            _nstore_cache=set(
                [(('host', 'localhost'), ('port', 13001), ('user', 'root'))]),
            nstore=[{"host": "localhost", "user": "root", "port": 13001}],
            append_password_list=[('store', 'root@localhost:13001')]))

    def test_add_stdin_password_option(self):
        """test add_stdin_password option"""
        options.add_stdin_password_option(self.parser)
        # if nothing is passed it should be False
        parsed = self.parser.parse_args("")
        self.assertEqual(argparse.Namespace(stdin_pw=False), parsed)
        # if you pass the option, then the value should be True
        parsed = self.parser.parse_args(["--stdin"])
        self.assertEqual(argparse.Namespace(stdin_pw=True), parsed)
        # if it receives values parser should exit with error code 2
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args(["--stdin=yes"])
        self.assertEqual(err.exception.code, 2)

    def test_read_passwords(self):
        """test read_passwords"""
        # we can only test read_passwords with passwords sent via stdin
        options.add_stdin_password_option(self.parser)
        options.add_append_connection_option(self.parser, ["--store"], "store")
        options.add_store_connection_option(self.parser, ["--source"],
                                            "source")
        parsed = self.parser.parse_args("--source=root@localhost:13001 "
                                        "--store=root@localhost "
                                        "--stdin".split())
        pw_input = StringIO("store_pass\nsource_pass\n")
        pw_output = StringIO()
        old_stdin = sys.stdin
        old_stdout = sys.stdout
        try:
            # temporarily replace sys.stdin and sys.stdout to allow passing
            # the passwords and capturing the read_passwords output.
            sys.stdin = pw_input
            sys.stdout = pw_output
            options.read_passwords(parsed)
        finally:
            sys.stdin = old_stdin
            sys.stdout = old_stdout
        # Check that passwords were read and namespace updated
        self.assertEqual(parsed, argparse.Namespace(
            _store_cache=set([(('host', 'localhost'), ('port', 3306),
                               ('user', 'root'))]),
            append_password_list=[('store', 'root@localhost')],
            source={'passwd': 'store_pass', 'host': 'localhost',
                    'user': 'root', 'port': 13001},
            stdin_pw=True,
            store=[{'passwd': 'source_pass', 'host': 'localhost',
                    'user': 'root', 'port': 3306}],
            store_password_list=[('source', 'root@localhost:13001')]))

        # check that output was printed
        self.assertEqual(
            pw_output.getvalue(),
            "Enter the password for source (root@localhost:13001): \n"
            "Enter the password for store (root@localhost): \n")
        pw_output.close()
        pw_input.close()

    def test_add_verbose_option(self):
        """test add_verbose option"""
        options.add_verbose_option(self.parser, "verbose")
        # if nothing is passed it should be False
        parsed = self.parser.parse_args("")
        self.assertEqual(argparse.Namespace(verbose=False), parsed)
        # if you pass the option, then the value should be True
        parsed = self.parser.parse_args(["--verbose"])
        self.assertEqual(argparse.Namespace(verbose=True), parsed)
        # if it receives no values, parser should exit with error code 2
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args(["--verbose=yes"])
        self.assertEqual(err.exception.code, 2)

    def test_add_version_option(self):
        """test add_version option"""
        options.add_version_option(self.parser)
        # if this option is not passed, Namespace should be empty
        parsed = self.parser.parse_args("")
        self.assertEqual(argparse.Namespace(), parsed)
        # if option is used, is parser should exit with error code 0
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args(["--version"])
        self.assertEqual(err.exception.code, 0)

    def test_add_store_user_option(self):
        """test add_version option"""
        options.add_store_user_option(self.parser, ["--user"], "user",
                                      required=False, ask_pass=False)
        # if it receives no values, parser should exit with error code 2
        with self.assertRaises(SystemExit) as err:
            self.parser.parse_args(["--user"])
        self.assertEqual(err.exception.code, 2)
        # option with a value
        parsed = self.parser.parse_args(["--user=admid"])
        self.assertEqual(argparse.Namespace(user={"user": "admid"}), parsed)

        options.add_store_user_option(self.parser, ["--user_pass"], "user")
        # option with a value and password
        options.add_stdin_password_option(self.parser)
        parsed = self.parser.parse_args("--user_pass=admid --stdin".split())

        pw_input = StringIO("store_pass\n")
        pw_output = StringIO()
        old_stdin = sys.stdin
        old_stdout = sys.stdout
        try:
            # temporarily replace sys.stdin and sys.stdout to allow passing
            # the passwords and capturing the read_passwords output.
            sys.stdin = pw_input
            sys.stdout = pw_output
            options.read_passwords(parsed)
        finally:
            sys.stdin = old_stdin
            sys.stdout = old_stdout
        self.assertEqual(
            argparse.Namespace(stdin_pw=True,
                               store_password_list=[("user", "admid")],
                               user={'user': 'admid', 'passwd': 'store_pass'}),
            parsed)

    def test_force_read_password(self):
        """test force_read_password"""
        replication_user = "replication_user"
        value = {"user": "rpl_user"}
        namespace = argparse.Namespace(
            source={"host": "localhost", "user": "root", "port": 3306},
            replication_user=value,
            store_password_list=[('source', 'root@localhost')])

        options.force_read_password(namespace, replication_user, value)
        sp_list = getattr(namespace, "store_password_list")
        self.assertListEqual(
            sp_list,
            [('source', 'root@localhost'),
             ('replication_user', {'user': 'rpl_user'})]
        )

    def test_read_extra_password(self):
        """test read_extra_password"""
        # we can only test read_extra_password with passwords sent via stdin
        pw_input = StringIO("GeorgeOrwell\n")
        pw_output = StringIO()
        old_stdin = sys.stdin
        old_stdout = sys.stdout
        try:
            # temporarily replace sys.stdin and sys.stdout to allow passing
            # the passwords and capturing the read_passwords output.
            sys.stdin = pw_input
            sys.stdout = pw_output
            read_pw = options.read_extra_password("prompt text",
                                                  read_from_stdin=True)
        finally:
            sys.stdin = old_stdin
            sys.stdout = old_stdout
        self.assertEqual(read_pw, "GeorgeOrwell")
        self.assertEqual(pw_output.getvalue(), "prompt text\n")

if __name__ == "__main__":
    # buffer=True hides the stderr and stdout of successful tests.
    unittest.main(buffer=True)
