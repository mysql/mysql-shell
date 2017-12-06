# -*- coding: utf-8 -*-
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
This file contains unit unittests for the connection_parser module.
"""
import os
import re
import unittest

from mysql_gadgets.common.connection_parser import (clean_IPv6,
                                                    hostname_is_ip,
                                                    parse_connection,
                                                    parse_server_address,
                                                    parse_user_host,
                                                    IPV4, IPV6, HN, ANY_LIKE,
                                                    _match)
from mysql_gadgets.exceptions import GadgetCnxFormatError


def _cnx_dict_to_str(info):
    """Convert dictionary with connection information to string.
    """
    result = []

    user = info['user']
    if ':' in user or '@' in user:
        user = u"'{0}'".format(user)
    result.append(user)

    passwd = info.get('passwd')
    if passwd:
        result.append(':')
        if ':' in passwd or '@' in passwd:
            passwd = u"'{0}'".format(passwd)
        result.append(passwd)

    result.append('@')
    result.append(info['host'])
    if info.get('port') is not None:
        result.append(':')
        result.append(str(info['port']))
    if info.get('unix_socket'):
        result.append(':')
        result.append(info['unix_socket'])

    return ''.join(result)


class TestParseConnection(unittest.TestCase):
    """Test parsing of connection strings.
    """
    # This list consists of valid input connection strings and expected
    # resulting string (dictionary converted to string) after being parsed.
    valid_specifiers = [
        ('myuser@localhost', 'myuser@localhost:3306'),
        ('myuser@localhost:3307', 'myuser@localhost:3307'),
        ('myuser@localhost:65535', 'myuser@localhost:65535'),
        ('myuser@localhost:0', 'myuser@localhost:0'),
        ('myuser:foo@localhost', "'myuser:foo'@localhost:3306"),
        ('myuser:foo@localhost:3308', "'myuser:foo'@localhost:3308"),
        ('myuser:@localhost', "'myuser:'@localhost:3306"),
        ('mysql-user:!#$-%&@localhost', "'mysql-user:!#$-%&'@localhost:3306"),
        ('"one:two":foo@localhost', "'\"one:two\":foo'@localhost:3306"),
        ("auser:'foo:bar'@localhost", "'auser:'foo:bar''@localhost:3306"),
        ("auser:'foo@bar'@localhost", "'auser:'foo@bar''@localhost:3306"),
        ("auser:foo'bar@localhost", "'auser:foo'bar'@localhost:3306"),
        ("foo'bar:auser@localhost", "'foo'bar:auser'@localhost:3306"),
        ('auser:foo"bar@localhost', '\'auser:foo"bar\'@localhost:3306'),
        ('foo"bar:auser@localhost', '\'foo"bar:auser\'@localhost:3306'),
        (u'ɱysql:unicode@localhost', u'\'ɱysql:unicode\'@localhost:3306'),
        # IPv6 strings
        ("user@3ffe:1900:4545:3:200:f8ff:fe21:67cf",
         "user@[3ffe:1900:4545:3:200:f8ff:fe21:67cf]:3306"),
        ("user@fe80:0000:0000:0000:0202:b3ff:fe1e:8329",
         "user@[fe80:0000:0000:0000:0202:b3ff:fe1e:8329]:3306"),
        ("user@fe80:0:0:0:202:b3ff:fe1e:8329",
         "user@[fe80:0:0:0:202:b3ff:fe1e:8329]:3306"),
        ("user@'fe80::202:b3ff:fe1e:8329'",
         "user@fe80::202:b3ff:fe1e:8329:3306"),
        ("user@'fe80::0202:b3ff:fe1e:8329'",
         "user@fe80::0202:b3ff:fe1e:8329:3306"),
        ('user@"FE80::0202:B3FF:FE1E:8329"',
         "user@FE80::0202:B3FF:FE1E:8329:3306"),
        ('user@1:2:3:4:5:6:7:8', 'user@[1:2:3:4:5:6:7:8]:3306'),
        ("user@'E3D7::51F4:9BC8:192.168.100.32'",
         'user@E3D7::51F4:9BC8:192.168.100.32:3306'),
        ("user@'E3D7::51F4:9BC8'", 'user@E3D7::51F4:9BC8:3306'),
    ]
    # Connection strings with sockets are only valid on posix operating systems
    if os.name == 'posix':
        valid_specifiers.extend([
            ('myuser@localhost:3308:/usr/var/mysqld.sock',
             'myuser@localhost:3308'),
        ])
    # Invalid connection strings that should generate a GadgetCnxFormatError.
    invalid_specificers = [
        'myuser@', '@localhost', 'user@what:is::::this?',
        'user@1:2:3:4:5:6:192.168.1.110',
        'user@E3D7::51F4:9BC8:192.168.100.32', 'user@"', 'user@""', "user@'",
        "user@''", 'unknown', 'user@localhost:65536', 'user@localhost:-1'
    ]

    def test_valid(self):
        """Test parsing of valid connection strings.
        """
        for source, expected in self.valid_specifiers:
            result = _cnx_dict_to_str(parse_connection(source))
            self.assertEqual(expected, result,
                             u"Connection string '{0}' parsed to {1}, "
                             u"expected {2}".format(source, result, expected))

        parse_connection('myuser@localhost:3306', options=[])

    def test_invalid(self):
        """Test parsing of invalid connection strings.

        A GadgetCnxFormatError should be raised if the connection string is
        invalid.
        """
        for cnx_str in self.invalid_specificers:
            # Expect GadgetCnxFormatError: Error parsing connection string.
            with self.assertRaises(GadgetCnxFormatError):
                parse_connection(cnx_str)

    def test_using_options(self):
        """Test parsing of connection strings using options.
        """
        # Parse connection using an invalid options object.
        # Only a warning is logged and no options are used.
        cnx_str = 'myuser@localhost:3333'
        expected = {'user': 'myuser', 'host': 'localhost',
                    'port': 3333}
        result = parse_connection(cnx_str, options=[])
        self.assertEqual(result, expected,
                         "Connection string '{0}' parsed to {1}, "
                         "expected {2}".format(cnx_str, result, expected))

        # Parse connection using options (valid dictionary).
        cnx_str = 'myuser@localhost:3306'
        test_opts = {
            'charset': 'test-charset',
            'ssl_cert': 'test-cert',
            'ssl_ca': 'test-ca',
            'ssl_key': 'test-key',
            'ssl': 'test-ssl',
        }
        expected = {'user': 'myuser', 'host': 'localhost',
                    'port': 3306}
        expected.update(test_opts)
        result = parse_connection(cnx_str, options=test_opts)
        self.assertEqual(result, expected,
                         "Connection string '{0}' parsed to {1}, "
                         "expected {2}".format(cnx_str, result, expected))


class TestIPParser(unittest.TestCase):
    """Test server address parsing.
    """

    def test_valid_server_address(self):
        """Test parsing of valid server addresses.
        """
        # List of IPv4 test cases.
        ipv4_test_cases = [
            ("127.0.0.1", ("127.0.0.1", None, None, IPV4)),
            ("127.0.0.1:3306", ("127.0.0.1", "3306", None, IPV4)),
            ("0.0.0.1", ("0.0.0.1", None, None, IPV4)),
            ("0.0.0.1:3306", ("0.0.0.1", "3306", None, IPV4)),
        ]

        # List of IPv6 test cases.
        ipv6_test_cases = [
            ("0:1:2:3:4:5:6:7", ("[0:1:2:3:4:5:6:7]", None, None, IPV6)),
            ("[0:1:2:3:4:5:6:7]", ("[0:1:2:3:4:5:6:7]", None, None, IPV6)),
            ("0:1:2:3:4:5:6:7:3306",
             ("[0:1:2:3:4:5:6:7]", "3306", None, IPV6)),
            ("[0:1:2:3:4:5:6:7]:3306",
             ("[0:1:2:3:4:5:6:7]", "3306", None, IPV6)),
            ("0:1:2:3:4:5:6:7:3306:/a/b/mysql.socket",
             ("[0:1:2:3:4:5:6:7]", "3306", "/a/b/mysql.socket", IPV6)),
            ("[0:1:2:3:4:5:6:7]:3306:/a/b/mysql.socket",
             ("[0:1:2:3:4:5:6:7]", "3306", "/a/b/mysql.socket", IPV6)),
            ("0::7", ("[0::7]", None, None, IPV6)),
            ("::7", ("[::7]", None, None, IPV6)),
            ("::", ("[::]", None, None, IPV6)),
            ("[::1]", ("[::1]", None, None, IPV6)),
            ("[::1]:", ("[::1]", None, None, IPV6)),
            ("[::1]:3306", ("[::1]", "3306", None, IPV6)),
            ("[::1]:/a/b/mysql.socket",
             ("[::1]", '', "/a/b/mysql.socket", IPV6)),
            ("[::1]:3306:/a/b/mysql.socket",
             ("[::1]", "3306", "/a/b/mysql.socket", IPV6)),
            ("2001:db8::1:2", ("[2001:db8::1:2]", None, None, IPV6)),
            ("2001:db8::1:2:3306", ("[2001:db8::1:2:3306]", None, None, IPV6)),
            ("[0001:0002:0003:0004:0005:0006:0007:0008]:3306",
             ("[0001:0002:0003:0004:0005:0006:0007:0008]", "3306", None,
              IPV6)),
            ("0001:0002:0003:0004:0005:0006:0007:0008:3306",
             ("[0001:0002:0003:0004:0005:0006:0007:0008]", "3306", None,
              IPV6)),
            ("[001:2:3:4:5:6:7:8]:3306",
             ("[001:2:3:4:5:6:7:8]", "3306", None, IPV6)),
            ("001:2:3:4:5:6:7:8:3306",
             ("[001:2:3:4:5:6:7:8]", "3306", None, IPV6)),
            ("[2001:db8::1:2]:3306", ("[2001:db8::1:2]", "3306", None, IPV6)),
            ("1234:abcd:abcd:abcd:abcd:abcd:abcd:abcd",
             ("[1234:abcd:abcd:abcd:abcd:abcd:abcd:abcd]", None, None, IPV6)),
            ("fedc:abcd:abcd:abcd:abcd:abcd:abcd:abcd",
             ("[fedc:abcd:abcd:abcd:abcd:abcd:abcd:abcd]", None, None, IPV6)),
            ("fedc::abcd", ("[fedc::abcd]", None, None, IPV6)),
            ("0a:b:c::d", ("[0a:b:c::d]", None, None, IPV6)),
            ("abcd:abcd::abcd", ("[abcd:abcd::abcd]", None, None, IPV6)),
            ("[0a:b:c::d]", ("[0a:b:c::d]", None, None, IPV6)),
            ("[abcd:abcd::abcd]", ("[abcd:abcd::abcd]", None, None, IPV6)),
            ("[0:1:2:3:4:5:6:7]:3306",
             ("[0:1:2:3:4:5:6:7]", "3306", None, IPV6)),
            ("[0:1:2:3:4:5::7]:3306",
             ("[0:1:2:3:4:5::7]", "3306", None, IPV6)),
            ("[0:1:2:3:4::6:7]:3306",
             ("[0:1:2:3:4::6:7]", "3306", None, IPV6)),
            ("[0:1:2:3::5:6:7]:3306",
             ("[0:1:2:3::5:6:7]", "3306", None, IPV6)),
            ("[0:1:2::4:5:6:7]:3306",
             ("[0:1:2::4:5:6:7]", "3306", None, IPV6)),
            ("[0:1::3:4:5:6:7]:3306",
             ("[0:1::3:4:5:6:7]", "3306", None, IPV6)),
            ("[0::2:3:4:5:6:7]:3306",
             ("[0::2:3:4:5:6:7]", "3306", None, IPV6)),
            ("[fe80::202:b3ff:fe1e:8329]:3306",
             ("[fe80::202:b3ff:fe1e:8329]", "3306", None, IPV6)),
            ("fe80::202:b3ff:fe1e:8329",
             ("[fe80::202:b3ff:fe1e:8329]", None, None, IPV6)),
            ("[0:1:2:3:4::7]:3306", ("[0:1:2:3:4::7]", "3306", None, IPV6)),
            ("[0:1:2:3::7]:3306", ("[0:1:2:3::7]", "3306", None, IPV6)),
            ("[0:1:2::7]:3306", ("[0:1:2::7]", "3306", None, IPV6)),
            ("[0:1::7]:3306", ("[0:1::7]", "3306", None, IPV6)),
            ("[0::7]:3306", ("[0::7]", "3306", None, IPV6))
        ]

        # List of hostname test cases.
        hn_test_cases = [
            ("localhost", ("localhost", None, None, HN)),
            ("localhost:3306", ("localhost", "3306", None, HN)),
            ("localhost:3306:/var/lib/mysql.sock",
             ("localhost", "3306", "/var/lib/mysql.sock", HN)),
            ("oracle.com", ("oracle.com", None, None, HN)),
            ("oracle.com:3306", ("oracle.com", "3306", None, HN)),
            ("www.oracle.com:3306", ("www.oracle.com", "3306", None, HN)),
            ("this-is-mysite.com", ("this-is-mysite.com", None, None, HN)),
            ("w-w.this-site.com-com:3306",
             ("w-w.this-site.com-com", "3306", None, HN)),
            ("0DigitatBegining", ("0DigitatBegining", None, None, HN)),
            ("0DigitatBegining:3306", ("0DigitatBegining", "3306", None, HN)),
            ("0-w-w.this-site.c-om", ("0-w-w.this-site.c-om", None, None, HN)),
            ("0-w-w.this-site.c-om:3306",
             ("0-w-w.this-site.c-om", "3306", None, HN)),
            ("ww..this-site.com", ("ww", "", "..this-site.com", HN)),
        ]

        # List test cases using the wildcard character '%'.
        any_like_test_cases = [
            ("%", ("%", None, None, ANY_LIKE)),
            ("dataserver%33066mysql.sock",
             ("dataserver%33066mysql.sock", None, None, ANY_LIKE)),
            ("dataserver%:3306", ("dataserver%", "3306", None, ANY_LIKE)),
            ("dataserver%:3306:/folder/mysql.sock",
             ("dataserver%", "3306", "/folder/mysql.sock", ANY_LIKE)),
            ("%.loc.gov", ("%.loc.gov", None, None, ANY_LIKE)),
            ("x.y.%", ("x.y.%", None, None, ANY_LIKE)),
            ("x.%.z", ("x.%.z", None, None, ANY_LIKE)),
            ("192.168.10.%", ("192.168.10.%", None, None, ANY_LIKE)),
            ("192.168.%.%", ("192.168.%.%", None, None, ANY_LIKE)),
            ("192.%.%.%", ("192.%.%.%", None, None, ANY_LIKE)),
            ("192.168.%.15", ("192.168.%.15", None, None, ANY_LIKE)),
            ("%.loc.gov:33", ("%.loc.gov", "33", None, ANY_LIKE)),
            ("_can_start_and_end_with_.start%with_",
             ("_can_start_and_end_with_.start%with_", None, None, ANY_LIKE)),
            ("data%server%:3306:/folder/mysql.sock",
             ("data%server%", '3306', "/folder/mysql.sock", ANY_LIKE)),
            ("data-server%:3306:/folder/mysql.sock",
             ("data-server%", '3306', "/folder/mysql.sock", ANY_LIKE)),
            ("data-%ser-ver%:3306:/folder/mysql.sock",
             ("data-%ser-ver%", '3306', "/folder/mysql.sock", ANY_LIKE)),
            ("data%%server%:3306:/folder/mysql.sock",
             ("data%%server%", '3306', "/folder/mysql.sock", ANY_LIKE)),
            ("%.this_company.com:3306:/folder/mysql.sock",
             ("%.this_company.com", '3306', "/folder/mysql.sock", ANY_LIKE)),
            ("%._%:3306:/folder/mysql.sock",
             ("%._%", '3306', "/folder/mysql.sock", ANY_LIKE)),
            ("_can_start_and_end_with_.start%with_:3306:/folder/mysql.sock",
             ("_can_start_and_end_with_.start%with_", '3306',
              '/folder/mysql.sock', ANY_LIKE)),
        ]

        # Create list of valid test cases.
        valid_test_cases = ipv4_test_cases
        valid_test_cases.extend(ipv6_test_cases)
        valid_test_cases.extend(hn_test_cases)
        valid_test_cases.extend(any_like_test_cases)

        for test_case in valid_test_cases:
            result_tuple = parse_server_address(test_case[0])
            expected_tuple = (test_case[1])
            # Check if result matches expected values.
            self.assertEqual(result_tuple, expected_tuple,
                             "Server address '{0}' parsed to {1}, expected "
                             "{2}".format(test_case[0], result_tuple,
                                          expected_tuple))

    def test_invalid_server_address(self):
        """Test parsing of invalid server addresses.
        """
        exception_msg_unparsed = "not parsed completely"
        exception_msg_cannot_parse = "cannot be parsed"

        bad_ipv4_test_cases = [
            ("127.0.0", exception_msg_cannot_parse),
            ("1234.0.0.1:3306", exception_msg_cannot_parse),
            ("0.0.0.a", exception_msg_cannot_parse),
            ("0.b.0.1:3306", exception_msg_cannot_parse),
            ("0.0.c.1:3306", exception_msg_cannot_parse),
            ("012.345.678.910", exception_msg_cannot_parse),
        ]

        bad_ipv6_test_cases = [
            ("127::2::1", exception_msg_cannot_parse),
            ("1234.12345::0.1:3306", exception_msg_cannot_parse),
            ("1:2:3:4.0", exception_msg_cannot_parse),
            ("1:abcdf::g", exception_msg_cannot_parse),
            ("[g:b:c:d:e:f]", exception_msg_cannot_parse),
        ]

        bad_hn_test_cases = [
            ("local#host:3306", exception_msg_unparsed),
            ("local#host:3306:/var/lib/mysql.sock", exception_msg_unparsed),
            ("oracle.com/mysql:3306", exception_msg_unparsed),
            ("yo@oracle.com:3306", exception_msg_unparsed),
            ("yo:pass@127.0.0.1", exception_msg_unparsed),
            ("-no this-mysite.com", exception_msg_cannot_parse),
        ]

        bad_any_like_test_cases = [
            ("dataserver%.com/mysql:3306", exception_msg_unparsed),
            ("dat?%:3306:/folder/mysql.sock", exception_msg_unparsed),
            ("dat?%-:3306:/folder/mysql.sock", exception_msg_unparsed),
            ("dat?%.:3306:/folder/mysql.sock", exception_msg_unparsed),
            ("-no this%mysite.com", exception_msg_cannot_parse),
            (".cannot.start.with.dot%", exception_msg_cannot_parse),
            ("%cannot.end.with.", exception_msg_cannot_parse),
            ("%cannot.start%with-", exception_msg_cannot_parse),
            ("x.y.%.", exception_msg_cannot_parse),
            ("x.y.%.:", exception_msg_cannot_parse),
            ("x.y.%:", exception_msg_cannot_parse),
        ]

        # Create list of invalid test cases.
        invalid_test_cases = bad_ipv4_test_cases
        invalid_test_cases.extend(bad_ipv6_test_cases)
        invalid_test_cases.extend(bad_hn_test_cases)
        invalid_test_cases.extend(bad_any_like_test_cases)

        for test_case in invalid_test_cases:
            with self.assertRaises(GadgetCnxFormatError) as err:
                parse_server_address(test_case[0])
            self.assertIn(test_case[1][0], err.exception.errmsg)


class TestOtherChecks(unittest.TestCase):
    """Test other check functions from the connection_parser module.
    """
    def test_hostname_is_ip(self):
        """Test hostname_is_ip function.
        """
        self.assertEqual(hostname_is_ip("127.0.0.1"), True)
        self.assertEqual(
            hostname_is_ip("[0001:0002:0003:0004:0005:0006:0007:0008]"),
            True)
        self.assertEqual(hostname_is_ip("localhost"), False)

    def test_clean_IPv6(self):
        """Test clean_IPv6 function.
        """
        self.assertEqual(clean_IPv6("[2001:db8::ff00:42:8329]"),
                         "2001:db8::ff00:42:8329")

    def test_match(self):
        """Test _match function with throw_error=False.
        """
        pattern_only_numbers = re.compile(r"(\d+)", re.VERBOSE)

        # Fails to match pattern and return false (not an Exception).
        result = _match(pattern_only_numbers, "not_number", throw_error=False)
        self.assertFalse(result, "Expected False for _match()")

        # Successfully match pattern and return matched subgroups (not None).
        result = _match(pattern_only_numbers, "1234", throw_error=False)
        self.assertIsNotNone(result, "Expected not None result for _match()")
        self.assertNotEqual(result, False, "Expected not False for _match()")

    def test_parse_user_host(self):
        """Tests parse_user_host method
        """
        self.assertEqual(parse_user_host("user@host"), ("user", "host"))
        self.assertEqual(parse_user_host("'user'@'host'"), ("user", "host"))
        with self.assertRaises(GadgetCnxFormatError) as err:
            parse_user_host("'user'")
        self.assertIn("Cannot parse", err.exception.errmsg)

if __name__ == "__main__":
    unittest.main()
