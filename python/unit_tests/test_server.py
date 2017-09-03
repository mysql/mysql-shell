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
Unit tests for mysql_gadgets.common.server module.
"""
import copy
import os.path
import sys
import time
import unittest

from mysql_gadgets.common import server, tools
from mysql_gadgets.exceptions import (GadgetError, GadgetQueryError,
                                      GadgetCnxFormatError, GadgetCnxInfoError,
                                      GadgetCnxError, GadgetServerError)
from mysql_gadgets.common.group_replication import GR_PLUGIN_NAME
from unit_tests.utils import (GadgetsTestCase, SERVER_CNX_OPT,
                              skip_if_not_GR_approved,)

if sys.version_info > (3, 0):
    # Python 3
    import configparser  # pylint: disable=E0401,C0411
else:
    # Python 2
    import ConfigParser as configparser  # pylint: disable=E0401,C0411


class TestServer(GadgetsTestCase):  # pylint: disable=R0904
    """Class to test mysql_gadgets.common.server module.
    """

    @property
    def num_servers_required(self):
        """Property defining the number of servers required by the test.
        """
        return 1

    def setUp(self):
        """ Setup base server connection (for all tests) .
        """
        self.server_cnx = {'conn_info': self.options[SERVER_CNX_OPT][0]}
        self.server = server.Server(self.server_cnx)
        self.server.connect()

    def tearDown(self):
        """ Disconnect base server (for all tests).
        """
        self.server.disconnect()

    def test_server(self):
        """Test Server object creation.

        This test check additional circumstances of the creation of the
        Server object.

        Note: Successful creation of the server object already done in the
        setUp() function and it is required for other tests.
        """
        # Create server object with no options.
        with self.assertRaises(GadgetCnxInfoError):
            # options.get("conn_info") cannot be None
            server.Server(None)

        # Missing mandatory options for "conn_info" (host, user or port).
        opt_dict = {"conn_info": {}}
        with self.assertRaises(GadgetCnxInfoError):
            server.Server(opt_dict)
        opt_dict = {"conn_info": {'user': 'myuser', 'port': 3306}}
        with self.assertRaises(GadgetCnxInfoError):
            server.Server(opt_dict)
        opt_dict = {"conn_info": {'host': 'myhost', 'port': 3306}}
        with self.assertRaises(GadgetCnxInfoError):
            server.Server(opt_dict)
        opt_dict = {"conn_info": {'user': 'myuser', 'host': 'myhost'}}
        with self.assertRaises(GadgetCnxInfoError):
            server.Server(opt_dict)

        # No mandatory option missing.
        opt_dict = {"conn_info": {'user': 'myuser', 'host': 'myhost',
                                  'port': 3306}}
        myserver = server.Server(opt_dict)
        self.assertIsInstance(myserver, server.Server,
                              "The object 'myserver' is not an instance"
                              "of Server.")

        # Use SSL options.
        opt_dict = {"conn_info": {'user': 'myuser', 'host': 'myhost',
                                  'port': 3306, 'ssl_ca': 'myca',
                                  'ssl_key': 'mykey', 'ssl_cert': 'mycert',
                                  'ssl': True}}
        myserver = server.Server(opt_dict)
        self.assertIsInstance(myserver, server.Server,
                              "The object 'myserver' is not an instance"
                              "of Server.")

    def test_connect_disconnect(self):
        """Test connect and disconnect methods.

        This test check additional use case to try to connect and disconnect
        to the server.

        Note: Successful connection to a server is already done in the
        setUp() function and it is required for other tests.
        """

        # Connect to the server.
        cnx_opt = copy.deepcopy(self.server_cnx)
        myserver = server.Server(cnx_opt)
        myserver.connect()
        self.assertTrue(myserver.is_alive(),
                        "The myserver was expected to be alive")

        # Try to connect again.
        myserver.connect()
        self.assertTrue(myserver.is_alive(),
                        "The myserver was expected to be alive")

        # Disconnect from the server.
        myserver.disconnect()
        self.assertFalse(myserver.is_alive(),
                         "The myserver was expected to be not alive")

        # Try to disconnect again.
        myserver.disconnect()
        self.assertFalse(myserver.is_alive(),
                         "The myserver was expected to be not alive")

        # Try to connect using wrong passwd.
        cnx_opt = copy.deepcopy(self.server_cnx)
        cnx_opt['conn_info'].update({'passwd': 'wrongpass'})
        myserver = server.Server(cnx_opt)
        with self.assertRaises(GadgetError):
            myserver.connect()

        # Try to connect to an invalid server.
        opt_dict = {"conn_info": {'user': 'myuser', 'host': 'myhost',
                                  'port': 3306, 'unix_socket': 'mysocket'}}
        myserver = server.Server(opt_dict)
        with self.assertRaises(GadgetError):
            myserver.connect()

        # Try to disconnect from an invalid server.
        with self.assertRaises(GadgetCnxError):
            myserver.disconnect()

        # Try to connect using fake SSL values.
        cnx_opt_ssl = copy.deepcopy(self.server_cnx)
        cnx_opt_ssl['conn_info'].update({'ssl_ca': 'myca', 'ssl_key': 'mykey',
                                         'ssl_cert': 'mycert', 'ssl': True})
        myserver = server.Server(cnx_opt_ssl)
        with self.assertRaises(GadgetError):
            myserver.connect()

        # Try to connect using previous fake SSL values and ssl_ca set to None.
        myserver.ssl_ca = None
        with self.assertRaises(GadgetError):
            myserver.connect()

    def test_get_connection_dictionary(self):
        """ Test get_connection_dictionary function.
        """
        # Test when conn_info is None.
        con_info = server.get_connection_dictionary(None)
        self.assertIsNone(con_info, "get_connection_dictionary() expected to "
                                    "return None")

        # Use a Server as parameter.
        # Note: Use fill connection information to ensure all is correctly
        # obtained from server instance using get_connection_values().
        opt_dict = {'conn_info': {'user': 'myuser', 'host': 'myhost',
                                  'port': 3306, 'passwd': 'mypasswd',
                                  'unix_socket': 'mysocket', 'ssl_ca': 'myca',
                                  'ssl_cert': 'mycert', 'ssl_key': 'mykey',
                                  'ssl': 'myssl'}}
        myserver = server.Server(opt_dict)
        con_info = server.get_connection_dictionary(myserver)
        self.assertEqual(con_info, opt_dict['conn_info'])

        # Use a string as parameter.
        opt_str = 'myuser@myhost:3306'
        con_info = server.get_connection_dictionary(opt_str)
        expected = {'user': 'myuser', 'host': 'myhost', 'port': 3306}
        self.assertEqual(con_info, expected)

        # Use SSL dict.
        cnx_dict = {'user': 'myuser', 'host': 'myhost', 'port': 3306}
        ssl_dict = {'ssl_ca': 'mycertificate'}
        con_info = server.get_connection_dictionary(cnx_dict,
                                                    ssl_dict=ssl_dict)
        expected = cnx_dict
        expected.update(ssl_dict)
        self.assertEqual(con_info, expected)

        # Invalid parameter type (using a tuple).
        with self.assertRaises(GadgetCnxInfoError):
            server.get_connection_dictionary(('mytuple',))

    def test_is_alive(self):
        """ Test is_alive function.
        """
        alive = self.server.is_alive()
        self.assertTrue(alive, "The server was expected to be alive")

    def test_is_alive_false(self):
        """ Test is_alive() function after disconnect.
        """
        server2 = server.Server(self.server_cnx)
        alive = server2.is_alive()
        self.assertFalse(alive, "The server was expected to be not alive")
        server2.connect()
        alive = server2.is_alive()
        self.assertTrue(alive, "The server was expected to be not alive")
        server2.disconnect()
        alive = server2.is_alive()
        self.assertFalse(alive, "The server was expected to be not alive")

    def test_get_version(self):
        """ Test get_version function.
        """
        # Get version from default server (clear cached version values first).
        self.server._version = None  # pylint: disable=W0212
        self.server._version_full = None  # pylint: disable=W0212
        version = self.server.get_version()
        self.assertIsInstance(version, list)
        self.assertEqual(len(version), 3)

        # Get version again, cached values are returned.
        version = self.server.get_version()
        self.assertIsInstance(version, list)
        self.assertEqual(len(version), 3)
        version = self.server.get_version(full=True)
        self.assertIsInstance(version, str)

        # Use a different Server object to get the full version, otherwise the
        # previously cached value is returned.
        cnx_opt = copy.deepcopy(self.server_cnx)
        myserver = server.Server(cnx_opt)
        myserver.connect()
        version = myserver.get_version(full=True)
        self.assertIsInstance(version, str)

        # Get version again, cached values are returned.
        version = myserver.get_version()
        self.assertIsInstance(version, list)
        self.assertEqual(len(version), 3)
        version = myserver.get_version(full=True)
        self.assertIsInstance(version, str)

        # Get version from a disconnected server.
        # Note: clear cached values to force trying to get version from server.
        myserver.disconnect()
        myserver._version = None  # pylint: disable=W0212
        myserver._version_full = None  # pylint: disable=W0212
        version = myserver.get_version()
        self.assertIsNone(version)

    def test_check_version_compat(self):
        """ Test check_version_compat function.
        """
        # Check version of connected server.
        result = self.server.check_version_compat(0, 0, 0)
        self.assertTrue(result)  # Server version is greater
        result = self.server.check_version_compat(99, 99, 99)
        self.assertFalse(result)  # Server version is lower

        # Check version for disconnected server (with cleared version cache)
        # Note: Cannot get version from server and return False.
        self.server.disconnect()
        self.server._version = None  # pylint: disable=W0212
        self.server._version_full = None  # pylint: disable=W0212
        result = self.server.check_version_compat(0, 0, 0)
        self.assertFalse(result)  # Server version is None, return False.
        result = self.server.check_version_compat(99, 99, 99)
        self.assertFalse(result)  # Server version is None, return False.

    def test_binlog_enabled(self):
        """ Test binlog_enabled function.
        """
        # Check if binlog is enabled (expected to be).
        res = self.server.binlog_enabled()
        if not res:
            raise unittest.SkipTest("Provided server must have binlog "
                                    "enabled.")
        self.assertTrue(res, "The server was expected to be binlog enabled")

        # Disconnect from server and try to check if binlog is enabled.
        self.server.disconnect()
        with self.assertRaises(GadgetServerError):
            self.server.binlog_enabled()

    def test_supports_plugin(self):
        """ Test supports_plugin function.
        """
        # Check valid active plugin.
        res = self.server.supports_plugin("InnoDB")
        self.assertTrue(res,
                        "The server is expected to support 'InnoDB' plugin")
        # Check valid but disabled plugin.
        res = self.server.supports_plugin("Federated")
        self.assertFalse(res,
                         "The 'Federated' plugin is expected to be disabled")
        # Check not supported or invalid plugin.
        res = self.server.supports_plugin("invalid_plugin")
        self.assertFalse(res,
                         "The 'invalid_plugin' plugin should not be supported")

        # get status for InnoDB plugin: must be ACTIVE by default
        res = self.server.supports_plugin("InnoDB", "DISABLED")
        self.assertFalse(res, "InnoDB plugin must not be DISABLED")

        # get status for FEDERATED plugin: must be DISABLED by default
        res = self.server.supports_plugin("FEDERATED", "DISABLED")
        self.assertTrue(res, "FEDERATED plugin must be DISABLED")

    def test_get_all_databases(self):
        """ Test get_all_databases function.
        """
        self.server.exec_query("drop database if exists db_test")
        self.server.exec_query("create database db_test")
        res = self.server.get_all_databases()
        self.server.exec_query("drop database if exists db_test")
        self.assertIn(("db_test",), res)

    def test_is_alias(self):
        """ Test is_alias function.
        """
        self.assertTrue(self.server.is_alias("127.0.0.1"))
        self.assertTrue(self.server.is_alias("0:0:0:0:0:0:0:1"))
        self.assertTrue(self.server.is_alias("::1"))
        # Use 'special' suffix for hostname.
        self.assertTrue(self.server.is_alias("localhost.local"))

    def test_is_alias_false(self):
        """ Test is_alias function when expecting a false value.
        """
        self.assertFalse(self.server.is_alias("216.58.218.206"))
        self.assertFalse(self.server.is_alias("0:0:0:0:0:0:0:2"))

    def test_from_server(self):
        """ Test fromServer function.
        """
        # Create new server from existing one.
        server2 = server.Server.from_server(self.server)
        self.assertTrue(
            server.check_hostname_alias(self.server.get_connection_values(),
                                        server2.get_connection_values())
        )
        server2.connect()
        self.assertTrue(server2.is_alive(),
                        "The server2 was expected to be alive")
        self.assertEqual(server2.get_server_id(), self.server.get_server_id())
        self.assertEqual(server2.get_server_uuid(),
                         self.server.get_server_uuid())
        server2.disconnect()
        self.assertFalse(server2.is_alive(),
                         "The server2 was expected to be not alive")

        # Use con_info parameter.
        server2 = server.Server.from_server(self.server,
                                            self.server_cnx['conn_info'])
        self.assertIsInstance(server2, server.Server,
                              "The object 'server2' is not an instance"
                              "of Server.")
        server2.connect()
        self.assertTrue(server2.is_alive(),
                        "The server2 was expected to be alive")
        server2.disconnect()
        self.assertFalse(server2.is_alive(),
                         "The server2 was expected to be not alive")

        # Use invalid server parameter (dictionary instead of Server instance).
        with self.assertRaises(TypeError):
            server.Server.from_server({})

    def test_get_server_id(self):
        """ Test get_server_id function.
        """
        # Get ID of connected server.
        self.assertIsNotNone(self.server.get_server_id(),
                             "Server UUID cannot be None")
        # Disconnect from server an try to get ID.
        self.server.disconnect()
        with self.assertRaises(GadgetServerError):
            self.server.get_server_id()

    def test_get_server_uuid(self):
        """ Test get_server_uuid function.
        """
        # Get UUID of connected server.
        self.assertIsNotNone(self.server.get_server_uuid(),
                             "Server UUID cannot be None")
        # Disconnect from server an try to get UUID.
        self.server.disconnect()
        with self.assertRaises(GadgetServerError):
            self.server.get_server_uuid()

    def test_server_str(self):
        """ Test str(server) function.
        """
        # Test without using unix socket.
        opt_dict = {"conn_info": {'user': 'myuser', 'host': 'myhost',
                                  'port': 3306}}
        myserver = server.Server(opt_dict)
        self.assertEqual("{0}".format(str(myserver)),
                         "'{0}@{1}'".format(myserver.host, myserver.port))

        # Test using unix socket (platform dependent, only for POSIX).
        if os.name == "posix":
            opt_dict = {"conn_info": {'user': 'myuser', 'host': 'myhost',
                                      'port': 3306, 'unix_socket': 'mysocket'}}
            myserver = server.Server(opt_dict)
            self.assertEqual("{0}".format(str(myserver)),
                             "'{0}@{1}'".format(myserver.host,
                                                myserver.socket))

    def test_get_server_binlogs_list(self):
        """ Test get_server_binlogs_list function.
        """
        if not self.server.binlog_enabled():
            raise unittest.SkipTest("Provided server must have binlog "
                                    "enabled.")
        binlogs_list = self.server.get_server_binlogs_list()
        # test binlogs_list is not empty list
        self.assertGreaterEqual(len(binlogs_list), 1)
        self.server.flush_logs()
        # This second call is just to get a higher coverage.
        self.server.flush_logs('BINARY')
        binlogs_list_a = self.server.get_server_binlogs_list(True)
        # test binlogs_list_a is not empty list
        self.assertGreaterEqual(len(binlogs_list_a), 1)
        # binlogs_list not empty
        _, bl_size = binlogs_list_a[0]
        # test binlogs has been flushed creating a new binlog
        self.assertGreater(len(binlogs_list_a), len(binlogs_list))
        # test binlog size is an int bigger or equal than 0
        self.assertGreaterEqual(int(bl_size), 0)

    def test_select_variable(self):
        """ Test select_variable function.
        """
        port = self.server.select_variable("port")
        self.assertEqual(port, repr(self.server.port))
        port = self.server.select_variable("port", 'global')
        self.assertEqual(port, repr(self.server.port))

        # expect GadgetError: Timeout executing query
        with self.assertRaises(GadgetServerError) as test_raises:
            self.server.select_variable("port", 'local')
        exception = test_raises.exception
        self.assertIn("Invalid variable type", exception.errmsg)

    def test_supports_gtid(self):
        """ Test supports_gtid function.
        """
        # We expect that default server supports GTIDs
        self.assertTrue(self.server.supports_gtid())

        # Disconnect from server and try to check if GTID is supported.
        self.server.disconnect()
        with self.assertRaises(GadgetServerError):
            self.server.supports_gtid()

    def test_exec_query(self):
        """ Test exec_query function.
        """
        # Execute statements using default setting and using params.
        self.server.exec_query('CREATE DATABASE mytestdb1234')
        self.server.exec_query(
            'CREATE TABLE mytestdb1234.foo (a INT, b VARCHAR(20))')
        self.server.exec_query(
            'INSERT INTO mytestdb1234.foo VALUES (1, "my test text")')
        res = self.server.exec_query("SELECT * FROM mytestdb1234.foo")
        self.assertEqual(res[0][0], '1')
        self.assertEqual(res[0][1], "my test text")
        res = self.server.exec_query(
            "SELECT * FROM mytestdb1234.foo WHERE %(a)s",
            {'params': {'a': 1}})
        self.assertEqual(res[0][0], '1')
        self.assertEqual(res[0][1], "my test text")
        res = self.server.exec_query(
            "SELECT * FROM mytestdb1234.foo WHERE %s",
            {'params': (1,)})
        self.assertEqual(res[0][0], '1')
        self.assertEqual(res[0][1], "my test text")
        self.server.exec_query('DROP DATABASE mytestdb1234')

        # Using raw set to False.
        options = {'raw': False}
        self.server.exec_query('CREATE DATABASE mytestdb1234', options)
        self.server.exec_query(
            'CREATE TABLE mytestdb1234.foo (a INT, b VARCHAR(20))', options)
        self.server.exec_query(
            'INSERT INTO mytestdb1234.foo VALUES (1, "my test text")', options)
        res = self.server.exec_query('SELECT * FROM mytestdb1234.foo', options)
        self.assertEqual(res[0][0], 1)
        self.assertEqual(res[0][1], "my test text")
        self.server.exec_query('DROP DATABASE mytestdb1234')

        # Using fetch set to False (return cursor for queries).
        options = {'fetch': False}
        cursor = self.server.exec_query('SHOW DATABASES', options)
        rows = cursor.fetchall()  # pylint: disable=E1101
        cursor = self.server.exec_query('SHOW DATABASES', options)
        for row in cursor:
            self.assertIn(row, rows)

        # Using columns set to True (also return column names).
        options = {'columns': True}
        res = self.server.exec_query('SHOW VARIABLES', options)
        self.assertListEqual(res[0], ['Variable_name', 'Value'])

        # Using commit set to False (not automatically performs commit)
        options = {'commit': False}
        self.server.exec_query('CREATE DATABASE mytestdb1234', options)
        self.server.exec_query(
            'CREATE TABLE mytestdb1234.foo (a INT, b VARCHAR(20))', options)
        self.server.exec_query(
            'INSERT INTO mytestdb1234.foo VALUES (1, "my test text")', options)
        self.server.commit()
        res = self.server.exec_query('SELECT * FROM mytestdb1234.foo', options)
        self.assertEqual(res[0][0], '1')
        self.assertEqual(res[0][1], "my test text")
        self.server.exec_query('DROP DATABASE mytestdb1234')

        # Execute an invalid query.
        with self.assertRaises(GadgetQueryError):
            self.server.exec_query('NOT A VALID QUERY')

    def test_rollback(self):
        """ Test rollback method.
        """
        # Using commit set to False to not automatically commit)
        options = {'commit': False}
        self.server.exec_query('CREATE DATABASE mytestdb1234', options)
        self.server.exec_query(
            'CREATE TABLE mytestdb1234.foo (a INT, b VARCHAR(20))', options)
        self.server.exec_query(
            'INSERT INTO mytestdb1234.foo VALUES (1, "my test text")', options)
        # Rollback INSERT.
        self.server.rollback()
        res = self.server.exec_query('SELECT * FROM mytestdb1234.foo', options)
        self.assertEqual(res, [])  # No value inserted
        self.server.exec_query('DROP DATABASE mytestdb1234')

    def test_exec_query_timeout(self):
        """ Test exec_query function with execution timeout.
        """
        # Query executes without being killed (taking less than 5 sec).
        res = self.server.exec_query("SHOW DATABASES", exec_timeout=5)
        self.assertGreater(len(res), 0,
                           "Number of rows (databases) should be > 0")

        # Query is killed because it reaches defined timeout (1 sec).
        with self.assertRaises(GadgetError) as err:
            self.server.exec_query("SELECT SLEEP(5)", exec_timeout=1)
        # expect GadgetError: Timeout executing query
        self.assertIn("Timeout executing query", err.exception.errmsg)

    def test_read_and_exec_from_file(self):
        """ Test read_and_exec_sql method.
        """
        self.server.exec_query("drop database if exists util_test")
        self.server.exec_query("drop user if exists joe_wildcard")
        self.server.exec_query("drop user if exists 'joe'@'user'")

        data_sql_path = os.path.normpath(
            os.path.join(__file__, "..", "std_data", "basic_data.sql"))

        self.server.read_and_exec_sql(data_sql_path, verbose=True)
        res = self.server.exec_query("select * from util_test.t1;")
        self.assertIn(('01 Test Basic database example',), res)
        # clean-up
        self.server.exec_query("drop database if exists util_test")
        self.server.exec_query("drop user if exists joe_wildcard")
        self.server.exec_query("drop user if exists 'joe'@'user'")

    def test_toggle_binlog(self):
        """ Test toggle_binlog method.
        """
        self.server.exec_query("SET SQL_LOG_BIN=0")
        self.server.toggle_binlog(action='enable')
        self.assertEqual(self.server.exec_query("show variables like"
                                                " '%sql_log_bin%'")[0][1],
                         'ON')
        self.server.toggle_binlog(action='disable')
        self.assertEqual(self.server.exec_query("show variables like"
                                                " '%sql_log_bin%'")[0][1],
                         'OFF')
        self.server.toggle_binlog(action='enable')
        self.assertEqual(self.server.exec_query("show variables like"
                                                " '%sql_log_bin%'")[0][1],
                         'ON')

    def test_grant_tables_enabled(self):
        """ Test grant_tables_enabled function.
        """
        res = self.server.grant_tables_enabled()
        self.assertTrue(res, "Server expected to be started without the "
                             "--skip-grant-tables option.")

    def test_user_host_exists(self):
        """ Test user_host_exist function.
        """
        # Check a non existing user (drop user first in case it exists).
        self.server.exec_query("DROP USER IF EXISTS joe_wildcard")
        self.assertFalse(self.server.user_host_exists('joe_wildcard', '%'))

        # Create a user with wildcard (%) host and check if it exists.
        # Note: User with the same name must match independently of the host.
        self.server.exec_query("CREATE USER 'joe_wildcard'@'%'")
        self.assertTrue(self.server.user_host_exists('joe_wildcard', '%'))
        self.assertTrue(self.server.user_host_exists('joe_wildcard',
                                                     'localhost'))

        # Create a user with a specific host and check if it exists.
        # Note: User must match both the user name and host part.
        self.server.exec_query("DROP USER IF EXISTS 'joe'@'user'")
        self.server.exec_query("CREATE USER 'joe'@'user'")
        self.assertTrue(self.server.user_host_exists('joe', 'user'))
        self.assertFalse(self.server.user_host_exists('joe', '%'))

        # Cleanup (drop created users).
        self.server.exec_query("DROP USER IF EXISTS joe_wildcard")
        self.server.exec_query("DROP USER IF EXISTS 'joe'@'user'")

    def test_to_config_file(self):
        """Test to_config_file method."""
        # Passing it a non Server object must yield an error
        class A(object):
            """Class a"""
            pass
        with self.assertRaises(TypeError):
            server.Server.to_config_file(A())

        # otherwise check that created file is correct
        fpath = server.Server.to_config_file(self.server, "testing")
        try:
            c = configparser.RawConfigParser()
            c.read(fpath)
            self.assertEqual(self.server.host, c.get("testing", "host"))
            self.assertEqual(self.server.port, int(c.get("testing", "port")))
            self.assertEqual(self.server.user, c.get("testing", "user"))
            if self.server.passwd:
                self.assertEqual(self.server.passwd, c.get("testing",
                                                           "password"))
            self.assertEqual("tcp", c.get("testing", "protocol"))
        finally:
            os.remove(fpath)

    def test_set_variable(self):
        """test set_variable function"""
        # passing an invalid var_type should yield an error
        with self.assertRaises(GadgetError):
            self.server.set_variable("name", "value", var_type="invalid_var")

        # Store old values.
        old_global_read_buffer_size = int(self.server.select_variable(
            "read_buffer_size", "global"))
        old_session_read_buffer_size = int(self.server.select_variable(
            "read_buffer_size", "session"))
        # read_buffer_size can be changed in multiples of 4K values (2**12)
        new_global_read_buffer_size = old_global_read_buffer_size + 2**12
        new_session_read_buffer_size = old_session_read_buffer_size + 2**13

        try:
            self.server.set_variable("read_buffer_size",
                                     str(new_global_read_buffer_size),
                                     "global")
            self.server.set_variable("read_buffer_size",
                                     str(new_session_read_buffer_size),
                                     "session")
            # Check that global and session variables were modified.
            self.assertEqual(new_global_read_buffer_size, int(
                self.server.select_variable("read_buffer_size", "global")))

            self.assertEqual(new_session_read_buffer_size, int(
                self.server.select_variable("read_buffer_size", "session")))

        finally:
            # Set the values again to old values
            self.server.set_variable("read_buffer_size",
                                     str(old_global_read_buffer_size),
                                     "global")
            self.server.set_variable("read_buffer_size",
                                     str(old_session_read_buffer_size),
                                     "session")

    def test_get_gtid_executed(self):
        """test set_gtid_executed method."""

        def fake_exec_query(*args, **kwargs):  # pylint: disable=W0613
            """Fake exec_query used to simulate the error thrown when asking
            for the value of the gtid_executed variable on a server wihout
            GTID support.
            """
            raise GadgetQueryError("Error message", "query")

        # store original methods to restore them later.
        old_supports_gtid = self.server.supports_gtid
        old_exec_query = self.server.exec_query

        # Monkey patch the supports_gtid method for the sake of this test
        # to fake a server without GTIDs enabled
        self.server.supports_gtid = lambda: False
        try:
            # using a server without gtid support should return error if
            # skip_gtid_check is False.
            with self.assertRaises(GadgetServerError):
                self.server.get_gtid_executed(skip_gtid_check=False)

            # If skip_gtid_check is enabled and we try to retrieve the
            # gtid_executed set of a server that does not support gtids, the
            # method must return an empty gtid_executed value.
            self.server.exec_query = fake_exec_query
            self.assertEqual(
                self.server.get_gtid_executed(skip_gtid_check=True), "")

            # If skip_gtid_check is disabled and we try to retrieve the
            # gtid_executed set of a server has gtids support then the
            # the exception thrown by the exec_query method must be re-raised.
            # Mock the supports_gtid method to have the behavior of a server
            # with GTID support enabled.
            self.server.supports_gtid = lambda: True
            with self.assertRaises(GadgetQueryError) as err:
                self.server.get_gtid_executed(skip_gtid_check=False)

            self.assertEqual(err.exception.errmsg, "Error message")
            self.assertEqual(err.exception.query, "query")

            # if the exec_query method returns a result with no rows, then
            # an empty string should be returned
            self.server.exec_query = lambda _: ""
            self.assertEqual(
                self.server.get_gtid_executed(skip_gtid_check=True), "")

        finally:
            # restore original supports_gtid method
            self.server.supports_gtid = old_supports_gtid
            self.server.exec_query = old_exec_query

    def test_toggle_global_read_lock(self):
        """test toggle_global_read_lock method."""
        has_super_read_only = self.server.check_version_compat(5, 7, 8)
        rd_only_name = "super_read_only" if has_super_read_only else \
            "read_only"
        rd_only = self.server.select_variable(rd_only_name, "global") == "ON"
        try:
            if rd_only:  # if super read_only is turned on, disable it
                self.server.toggle_global_read_lock(False)
                self.assertEqual(
                    self.server.select_variable(rd_only_name, "global"),
                    "0")
                # and enable it again
                self.server.toggle_global_read_lock(True)
                self.assertEqual(
                    self.server.select_variable(rd_only_name, "global"),
                    "1")
            else:
                self.server.toggle_global_read_lock(True)
                self.assertEqual(
                    self.server.select_variable(rd_only_name, "global"),
                    "1")
                self.server.toggle_global_read_lock(False)
                self.assertEqual(
                    self.server.select_variable(rd_only_name, "global"),
                    "0")
        finally:
            # set read_lock to initial state
            self.server.toggle_global_read_lock(rd_only)

    def test_install_plugin(self):
        """Test the install_plugin method"""
        skip_if_not_GR_approved(self.server)
        if self.server.is_plugin_installed(GR_PLUGIN_NAME):
            self.server.exec_query("UNINSTALL PLUGIN group_replication")

        self.server.install_plugin(GR_PLUGIN_NAME)
        self.assertTrue(self.server.is_plugin_installed(GR_PLUGIN_NAME),
                        "GR plugin was expected to be loaded")
        self.server.stop_plugin(GR_PLUGIN_NAME)
        self.assertTrue(self.server.is_plugin_installed(GR_PLUGIN_NAME),
                        "GR plugin was expected to be installed")

        self.server.uninstall_plugin(GR_PLUGIN_NAME)
        self.assertFalse(self.server.is_plugin_installed(GR_PLUGIN_NAME,
                                                         is_active=True))

        # Expect the plugin to start due to the missing configuration
        with self.assertRaises(GadgetQueryError):
            self.server.start_plugin(GR_PLUGIN_NAME)

        # User without Create User privileges
        self.server.exec_query("drop user if exists 'nop_user'@'localhost'")
        self.server.exec_query("create user nop_user@'localhost'")
        server_conn2 = self.server_cnx.copy()
        server_conn2["conn_info"] = ("nop_user@localhost:{0}"
                                     "".format(self.server.port))
        server2 = server.Server(self.server_cnx)
        server2.connect()
        # Test re-attempt to plugin uninstall after not been installed again
        self.server.uninstall_plugin(GR_PLUGIN_NAME)

        server2.install_plugin(GR_PLUGIN_NAME)
        self.assertTrue(self.server.is_plugin_installed(GR_PLUGIN_NAME),
                        "GR plugin was expected to be installed")

    def test_get_server(self):
        """Tests the get_server method.
        """
        ssl_dict = {}
        connect = False
        # get_server from connection information given as dictionary
        new_server = server.get_server(self.server_cnx["conn_info"], ssl_dict,
                                       connect)
        self.assertIsInstance(new_server, server.Server)
        # get_server from a Server instance
        new_server = server.get_server(self.server, ssl_dict, connect=True)
        self.assertIsInstance(new_server, server.Server)

        self.assertIsNone(server.get_server(None, ssl_dict, connect=False))

        with self.assertRaises(GadgetCnxFormatError):
            server.get_server("localhost:3396", ssl_dict, connect=False)

        with self.assertRaises(GadgetCnxInfoError):
            server.get_server(3396, ssl_dict, connect=False)

    def test_get_server_version(self):
        """Tests the get_server_version method."""
        # get version from the server
        version_from_server = self.server.get_version()
        # try to find the executable path via basedir
        basedir = self.server.select_variable("basedir", "global")
        try:
            mysqld_path = tools.get_tool_path(basedir, "mysqld", required=True)
        except GadgetError:
            raise unittest.SkipTest("Couldn't find mysqld executable")
        else:
            version_from_binary = server.get_mysqld_version(mysqld_path)
            self.assertEqual(tuple(version_from_server),
                             version_from_binary[0])
            self.assertIn(".".join(str(i) for i in version_from_binary[0]),
                          version_from_binary[1])

    def test_generate_server_id(self):
        """Tests generate_server_id() function.
        """
        # Test server_id generation using 'RANDOM' strategy.
        # Validate range and that two generated IDs are different.
        srv_id = server.generate_server_id()
        self.assertGreaterEqual(int(srv_id), 1)
        self.assertLessEqual(int(srv_id), 4294967295)

        srv_id2 = server.generate_server_id('RANDOM')
        self.assertGreaterEqual(int(srv_id2), 1)
        self.assertLessEqual(int(srv_id2), 4294967295)
        self.assertNotEqual(int(srv_id), int(srv_id2))

        # Test server_id generation using 'TIME' strategy.
        # Validate range and that two generated IDs are different.
        srv_id = server.generate_server_id('TIME')
        self.assertGreaterEqual(int(srv_id), 1)
        self.assertLessEqual(int(srv_id), 999999999)

        time.sleep(1)  # Wait 1 sec to generate a different ID.
        srv_id2 = server.generate_server_id('TIME')
        self.assertGreaterEqual(int(srv_id2), 1)
        self.assertLessEqual(int(srv_id2), 999999999)
        self.assertNotEqual(int(srv_id), int(srv_id2))

        # Error raised if an unknown strategy is used.
        with self.assertRaises(GadgetError):
            server.generate_server_id('New')
