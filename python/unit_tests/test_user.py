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
Tests mysql_gadgets.common.user
"""

from mysql_gadgets.common.server import Server
from mysql_gadgets.common.user import (change_user_privileges,
                                       check_privileges,
                                       User,)
from mysql_gadgets.exceptions import GadgetError

from unit_tests.utils import GadgetsTestCase, SERVER_CNX_OPT


class Test(GadgetsTestCase):
    """Class to test mysql_gadgets.common.user module.
    """

    @property
    def num_servers_required(self):
        """Property defining the number of servers required by the test.
        """
        return 1

    def setUp(self):
        """ Setup server connection
        """
        self.server_cnx = {'conn_info': self.options[SERVER_CNX_OPT][0]}
        self.server = Server(self.server_cnx)
        self.server.connect()

    def tearDown(self):
        """ Disconnect base server (for all tests).
        """
        self.server.disconnect()

    def test_create_user(self):
        """ Tests  User.create_user methods"""
        self.server.exec_query("Drop USER if exists 'joe'@'users'")
        self.server.exec_query("Drop USER if exists 'Jonh_CAPITALS'@'{0}'"
                               "".format(self.server.host))

        user_root = User(self.server, "{0}@{1}".format(self.server.user,
                                                       self.server.host))

        user_name = 'Jonh_CAPITALS'
        user_root.create(new_user="{0}@{1}".format(user_name,
                                                   self.server.host),
                         passwd="his_pass", ssl=True, disable_binlog=True)

        user_root.exists("{0}@{1}".format(user_name, self.server.host))

        user_obj2 = User(self.server, "{0}@{1}".format('joe', 'users'))

        user_root.drop(new_user="{0}@{1}".format(user_name, self.server.host))
        user_obj2.drop()

        self.assertFalse((self.server.user_host_exists('Jonh_CAPITALS',
                                                       self.server.host)))

    def test_check_privileges(self):
        """ Tests check_privileges"""
        self.server.exec_query("Drop USER if exists 'check_privs'@'%'")
        self.server.exec_query("CREATE USER 'check_privs'@'%'")
        operation = "can select?"
        privileges = ['SELECT']
        description = "checking privs"
        server1 = Server({"conn_info": "check_privs@localhost:{0}"
                                       "".format(self.server.port)})
        server1.connect()
        # expect GadgetError: Query failed: Unknown system variable
        with self.assertRaises(GadgetError) as test_raises:
            check_privileges(server1, operation, privileges, description)
        exception = test_raises.exception
        self.assertTrue("not have sufficient privileges" in exception.errmsg,
                        "The exception message was not the expected: {0}"
                        "".format(exception.errmsg))

        self.server.exec_query("Drop USER if exists 'check_privs'@'%'")

    def test_check_missing_privileges(self):
        """ Tests the User's check_missing_privileges method"""

        self.server.exec_query("Drop USER if exists 'Jonh_CAPITALS'@'{0}'"
                               "".format(self.server.host))
        user_name = 'Jonh_CAPITALS'
        user_obj = User(self.server, "{0}@{1}".format(user_name,
                                                      self.server.host),
                        verbosity=1)
        self.assertFalse(user_obj.exists())

        user_obj.create(disable_binlog=True, ssl=True)
        self.assertTrue(user_obj.exists())

        self.assertListEqual(
            user_obj.check_missing_privileges(
                ["REPLICATION SLAVE", "CREATE USER"], as_string=False),
            ["CREATE USER", "REPLICATION SLAVE"])

        self.assertEqual(
            user_obj.check_missing_privileges(["REPLICATION SLAVE",
                                               "CREATE USER"]),
            "CREATE USER and REPLICATION SLAVE")

        change_user_privileges(self.server, user_name, self.server.host,
                               grant_list=["REPLICATION SLAVE", "CREATE USER"],
                               disable_binlog=True)
        self.assertListEqual(user_obj.get_grants(refresh=True),
                             [("GRANT REPLICATION SLAVE, CREATE USER ON *.* "
                               "TO 'Jonh_CAPITALS'@'localhost'",)])
        self.assertListEqual(
            user_obj.check_missing_privileges(["REPLICATION SLAVE",
                                               "CREATE USER"],
                                              as_string=False), [])

        self.assertEqual(
            user_obj.check_missing_privileges(["REPLICATION SLAVE",
                                               "CREATE USER"]), "")

        user_obj.drop()

        user_name = 'some_user'
        self.server.exec_query("Drop USER if exists '{0}'@'{1}'"
                               "".format(user_name, self.server.host))
        change_user_privileges(self.server, user_name, self.server.host,
                               user_passwd="some pass",
                               grant_list=["REPLICATION SLAVE", "CREATE USER"],
                               disable_binlog=True, create_user=True)
        change_user_privileges(self.server, user_name, self.server.host,
                               revoke_list=["REPLICATION SLAVE"],
                               disable_binlog=True)

        # expect GadgetError: Query failed: Unknown system variable
        with self.assertRaises(GadgetError) as test_raises:
            user_obj.drop(user_name)
        exception = test_raises.exception
        self.assertTrue("Cannot parse user@host" in exception.errmsg,
                        "The exception message was not the expected")
        user_obj.drop("{0}@{1}".format(user_name, self.server.host))
        self.server.exec_query("Drop USER if exists '{0}'@'{1}'"
                               "".format(user_name, self.server.host))

    def test_get_grants(self):
        """ Tests get_grants method"""
        self.server.exec_query("Drop USER if exists '{0}'@'{1}'"
                               "".format('jose', '%'))
        self.server.exec_query("Drop USER if exists '{0}'@'{1}'"
                               "".format('jose', self.server.host))
        user_obj2 = User(self.server, "{0}@{1}".format('jose', '%'),
                         verbosity=True)
        self.assertListEqual(user_obj2.get_grants(globals_privs=True), [])
        user_obj2.create()
        # Test user has none grants
        self.assertListEqual(user_obj2.get_grants(globals_privs=True),
                             [("GRANT USAGE ON *.* TO 'jose'@'%'",)])
        self.assertDictEqual(user_obj2.get_grants(as_dict=True),
                             {'*': {'*': {'USAGE'}}})

        # Test get global privileges
        self.server.exec_query("GRANT PROXY ON '{0}'@'{1}' TO '{0}'@'%'"
                               "".format('jose', self.server.host))
        self.server.exec_query("GRANT UPDATE ON mysql.* TO '{0}'@'{1}'"
                               "".format('jose', '%'))
        exp_list_res = [("GRANT USAGE ON *.* TO 'jose'@'%'",),
                        ("GRANT UPDATE ON `mysql`.* TO 'jose'@'%'",),
                        ("GRANT PROXY ON 'jose'@'{0}' TO 'jose'@'%'"
                         "".format(self.server.host),)]
        self.assertListEqual(user_obj2.get_grants(globals_privs=True,
                                                  refresh=True),
                             exp_list_res)
        self.assertDictEqual(user_obj2.get_grants(as_dict=True, refresh=True),
                             {'*': {'*': {'USAGE'}},
                              '`mysql`': {'*': {'UPDATE'}}})

        user_obj2.drop()
        self.server.exec_query("Drop USER if exists '{0}'@'{1}'"
                               "".format('jose', '%'))
        self.server.exec_query("Drop USER if exists '{0}'@'{1}'"
                               "".format('jose', self.server.host))

    def test_user_has_privilege(self):
        """ Tests USER.has_privilege method"""
        user_name = 'Jonh_Update'
        user_root = User(self.server, "{0}@{1}".format(self.server.user,
                                                       self.server.host))
        self.server.exec_query("Drop USER if exists '{0}'@'{1}'"
                               "".format(user_name, self.server.host))

        db_ = "mysql"
        obj = "user"
        access = "UPDATE"
        skip_grant = False
        # Test object level privilege with user with global * privilege
        self.assertTrue(user_root.has_privilege(db_, obj, access, skip_grant))

        # create new user to test missing privileges.
        user_update = User(self.server, "{0}@{1}".format(user_name,
                                                         self.server.host))
        user_update.create()
        # Test privileges disabled
        self.server.grants_enabled = False
        skip_grant = True
        self.assertTrue(user_update.has_privilege(db_, obj, access,
                                                  skip_grant))

        self.server.grants_enabled = True
        skip_grant = True
        # Test missing privilege
        self.assertFalse(user_update.has_privilege(db_, obj, access,
                                                   skip_grant))

        access = "USAGE"
        # Test default privilege USAGE at db level
        self.assertTrue(user_update.has_privilege(db_, obj, access,
                                                  skip_grant))

        self.server.exec_query("GRANT UPDATE ON mysql.user TO '{0}'@'{1}'"
                               "".format(user_name, self.server.host))
        access = "UPDATE"
        # Test privilege at object level
        self.assertTrue(user_update.has_privilege(db_, obj, access,
                                                  skip_grant))

        # Test privilege at db level
        self.server.exec_query("GRANT UPDATE ON mysql.* TO '{0}'@'{1}'"
                               "".format(user_name, self.server.host))
        self.assertTrue(user_update.has_privilege(db_, obj, access,
                                                  skip_grant))

        user_update.drop()

    def test_parse_grant_statement(self):
        """ Tests parse_grant_statement Method.
        """
        # Test function
        parsed_statement = User.parse_grant_statement(
            "GRANT ALTER ROUTINE, EXECUTE ON FUNCTION `util_test`.`f1` TO "
            "'priv_test_user2'@'%' WITH GRANT OPTION")
        self.assertEqual(parsed_statement,
                         (set(['GRANT OPTION', 'EXECUTE', 'ALTER ROUTINE']),
                          None, '`util_test`', '`f1`',
                          "'priv_test_user2'@'%'"))
        # Test procedure
        parsed_statement = User.parse_grant_statement(
            "GRANT ALTER ROUTINE ON PROCEDURE `util_test`.`p1` TO "
            "'priv_test_user2'@'%' IDENTIFIED BY "
            "PASSWORD '*123DD712CFDED6313E0DDD2A6E0D62F12E580A6F' "
            "WITH GRANT OPTION")
        self.assertEqual(parsed_statement,
                         (set(['GRANT OPTION', 'ALTER ROUTINE']), None,
                          '`util_test`', '`p1`', "'priv_test_user2'@'%'"))
        # Test with quoted objects
        parsed_statement = User.parse_grant_statement(
            "GRANT CREATE VIEW ON `db``:db`.```t``.``export_2` TO "
            "'priv_test_user'@'%'")
        self.assertEqual(parsed_statement,
                         (set(['CREATE VIEW']), None, '`db``:db`.```t``',
                          '``export_2`', "'priv_test_user'@'%'"))
        parsed_statement = User.parse_grant_statement(
            "GRANT CREATE VIEW ON `db``:db`.```t``.* TO "
            "'priv_test_user'@'%'")
        self.assertEqual(parsed_statement,
                         (set(['CREATE VIEW']), None, '`db``:db`.```t``', '*',
                          "'priv_test_user'@'%'"))
        # Test multiple grants with password and grant option
        parsed_statement = User.parse_grant_statement(
            "GRANT UPDATE, SELECT ON `mysql`.* TO 'user2'@'%' IDENTIFIED BY "
            "PASSWORD '*123DD712CFDED6313E0DDD2A6E0D62F12E580A6F' "
            "REQUIRE SSL WITH GRANT OPTION")
        self.assertEqual(parsed_statement,
                         (set(['GRANT OPTION', 'UPDATE', 'SELECT']), None,
                          '`mysql`', '*', "'user2'@'%'"))
        parsed_statement = User.parse_grant_statement(
            "GRANT UPDATE, SELECT ON `mysql`.* TO 'user2'@'%' IDENTIFIED BY "
            "PASSWORD REQUIRE SSL WITH GRANT OPTION")
        self.assertEqual(parsed_statement,
                         (set(['GRANT OPTION', 'UPDATE', 'SELECT']), None,
                          '`mysql`', '*', "'user2'@'%'"))
        # Test proxy privileges
        parsed_statement = User.parse_grant_statement(
            "GRANT PROXY ON ''@'' TO 'root'@'localhost' WITH GRANT OPTION")
        self.assertEqual(parsed_statement,
                         (set(['GRANT OPTION', 'PROXY']), "''@''", None, None,
                          "'root'@'localhost'"))
        parsed_statement = User.parse_grant_statement(
            "GRANT PROXY ON 'root'@'%' TO 'root'@'localhost' WITH GRANT "
            "OPTION")
        self.assertEqual(parsed_statement,
                         (set(['GRANT OPTION', 'PROXY']), "'root'@'%'", None,
                          None, "'root'@'localhost'"))
        # Test parse grant with ansi quotes
        parsed_statement = User.parse_grant_statement(
            "GRANT UPDATE, SELECT ON mysql.user TO user2@'%'",
            sql_mode="ANSI_QUOTES")
        self.assertEqual(parsed_statement[4], ("user2@'%'"))
        parsed_statement = User.parse_grant_statement(
            "GRANT UPDATE, SELECT ON mysql.user TO user2@'%'")
        self.assertEqual(parsed_statement[2], '`mysql`')
        parsed_statement = User.parse_grant_statement(
            "GRANT UPDATE, SELECT ON mysql.user TO user2@'%'")
        self.assertEqual(parsed_statement[3], '`user`')

        self.assertRaises(GadgetError, User.parse_grant_statement,
                          "GRANT PROXY 'root'@'%' TO 'root'@'localhost' WITH "
                          "GRANT OPTION")
