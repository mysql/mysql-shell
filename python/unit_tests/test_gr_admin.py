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
Tests for mysql_gadgets.command.gr_admin
"""

import unittest
from unit_tests.mock_server import get_mock_server
from unit_tests.utils import (GadgetsTestCase, get_free_random_port,
                              SERVER_CNX_OPT, skip_if_not_GR_approved)

from mysql_gadgets.common.server import Server
from mysql_gadgets.common.group_replication import (HAVE_SSL,
                                                    is_member_of_group,
                                                    REP_CONN_STATUS_TABLE,
                                                    REP_GROUP_MEMBERS_TABLE,
                                                    REP_MEMBER_STATS_TABLE)
from mysql_gadgets.command.gr_admin import (start, join, health, leave,
                                            MEMBER_ID, MEMBER_HOST,
                                            MEMBER_PORT, MEMBER_STATE,
                                            resolve_gr_local_address,)
from mysql_gadgets.common.user import change_user_privileges
from mysql_gadgets.exceptions import GadgetError


class TestGRAdmin(GadgetsTestCase):
    """This class tests the methods in mysql_gadgets.command.gr_admin
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

        skip_if_not_GR_approved(self.server)

        if self.server.select_variable(HAVE_SSL) != 'YES':
            raise unittest.SkipTest("Provided server must have_ssl == 'YES'.")

        # User without Create User privileges
        self.server.exec_query("drop user if exists 'nop_user'@'localhost'")
        self.server.exec_query("create user nop_user@'localhost'")
        server_conn2 = self.server_cnx.copy()
        server_conn2["conn_info"] = ("nop_user@localhost:{0}"
                                     "".format(self.server.port))
        self.server2 = Server(self.server_cnx)
        self.server2.connect()

        columns = [MEMBER_ID, MEMBER_HOST, MEMBER_PORT, MEMBER_STATE]
        qry_key = "SELECT {0} FROM {1}".format(", ".join(columns),
                                               REP_GROUP_MEMBERS_TABLE)
        qry_key2 = ("SELECT GROUP_NAME FROM {0} where "
                    "CHANNEL_NAME = 'group_replication_applier'"
                    "".format(REP_CONN_STATUS_TABLE))
        frozen_queries = {
            qry_key: [[], []],
            qry_key2: [],
        }
        self.mock_no_member = get_mock_server(self.server,
                                              queries=frozen_queries)

    def tearDown(self):
        """ Disconnect base server (for all tests).
        """
        # reconnect the server.
        self.server.connect()

        leave(self.server)

        # reconnect the server.
        self.server.connect()

        self.server.exec_query("drop user if exists 'rpl_user'")
        self.server.exec_query("drop user if exists 'nop_user'@'localhost'")
        self.server.exec_query("drop user if exists 'new_rpl_user'")
        self.server.disconnect()
        self.server2.disconnect()

    def test_bootstrap(self):
        """Tests start method
        """
        # Fill the options
        options = {
            "group_name": None,
            "replication_user": None,
            "rep_user_passwd": "passwd",
            "verbose": True
        }

        # test start
        if is_member_of_group(self.server):
            leave(self.server, **options)

        # reconnect the server.
        self.server.connect()

        self.assertTrue(start(self.server, **options))

        # reconnect the server.
        self.server.connect()

        # health
        health(self.server, **options)

        # reconnect the server.
        self.server.connect()

        # Bootstrap active server
        # Trying to start a group with a server that is already a member
        # expect GadgetError: Query failed: is already a member of a GR group.
        with self.assertRaises(GadgetError) as test_raises:
            start(self.server, **options)
        exception = test_raises.exception
        self.assertTrue("is already a member" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        self.server.connect()

        leave(self.server, **options)

        # reconnect the server.
        self.server.connect()

        group_name = "b7286041-3016-11e6-ba52-507b9d87510a"
        # start using not defaults
        options = {
            "group_name": "b7286041-3016-11e6-ba52-507b9d87510a",
            "gr_host": "{0}:".format(self.server.host),
            "replication_user": "replicator_user@'%'",
            "rep_user_passwd": "passwd",
            "verbose": True,
            "dry_run": False,
        }

        self.assertTrue(start(self.server, **options))
        # reconnect the server.
        self.server.connect()
        self.assertTrue(is_member_of_group(self.server, group_name))

        self.assertFalse(is_member_of_group(self.server, "group_name"))

        if is_member_of_group(self.server):
            leave(self.server, **options)

        # reconnect the server.
        self.server.connect()

    def test_bootstrap_noselect_priv(self):
        """Tests commands without Select privilege.
        """
        self.server.exec_query("drop user if exists 'not_select'@'%'")
        change_user_privileges(self.server, "not_select", "%",
                               user_passwd="pass", create_user=True)

        server1 = Server({
            "conn_info": "not_select@localhost:{0}".format(self.server.port)})
        server1.passwd = "pass"
        server1.connect()
        # Fill the options
        options = {
            "group_name": "b7286041-3016-11e6-ba52-507b9d87510a",
            "gr_host": ":{0}".format(self.server.port),
            "replication_user": "new_user@'%'",
            "rep_user_passwd": "passwd",
            "verbose": True,
            "dry_run": False,
        }

        # expect GadgetError: Query failed: not have enough privileges to
        #                                   verify server
        with self.assertRaises(GadgetError) as test_raises:
            leave(server1, **options)
        exception = test_raises.exception
        self.assertTrue("not have enough privileges to verify server" in
                        exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        server1.connect()

        # expect GadgetError: Query failed: not have enough privileges to
        #                                   verify server
        with self.assertRaises(GadgetError) as test_raises:
            start(server1, **options)
        exception = test_raises.exception
        self.assertTrue("The operation could not continue due to" in
                        exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        server1.connect()

        # expect GadgetError: Query failed: not have enough privileges to
        #                                   verify server
        with self.assertRaises(GadgetError) as test_raises:
            health(server1, **options)
        exception = test_raises.exception
        self.assertTrue("not have enough privileges to verify server" in
                        exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        server1.connect()

        # expect GadgetError: Query failed: not have enough privileges to
        #                                   verify server
        with self.assertRaises(GadgetError) as test_raises:
            leave(server1, **options)
        exception = test_raises.exception
        self.assertTrue("not have enough privileges to verify server" in
                        exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

    def test_re_bootstrap(self):
        """Tests start method over actively replicating server
        """
        # Fill the options
        options = {
            "group_name": None,
            "replication_user": None,
            "rep_user_passwd": "passwd",
            "gr_host": "",
        }

        # test start
        if is_member_of_group(self.server):
            leave(self.server, **options)

        # reconnect the server.
        self.server.connect()

        self.assertTrue(start(self.server, **options))

        # reconnect the server.
        self.server.connect()

        # test health
        health(self.server, **options)

        # reconnect the server.
        self.server.connect()

        # Trying to start a group with a server that is already a member
        # expect GadgetError: Query failed: is already a member of a GR group.
        with self.assertRaises(GadgetError) as test_raises:
            start(self.server, **options)
        exception = test_raises.exception
        self.assertTrue("is already a member" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        self.server.connect()

        # Bootstrap with dry-run
        options["dry_run"] = True
        # Trying to start a group with a server that is already a member
        # expect GadgetError: Query failed: is already a member of a GR group.
        with self.assertRaises(GadgetError) as test_raises:
            start(self.server, **options)
        exception = test_raises.exception
        self.assertTrue("is already a member" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        self.server.connect()

        self.assertTrue(leave(self.server, **options))

        # reconnect the server.
        self.server.connect()

    def test_join(self):
        """Tests join method
        """
        # Fill the options
        options = {
            "group_name": None,
            "gr_host": "{0}:{1}".format(self.server.host, self.server.port),
            "replication_user": "replicator_user@'%'",
            "rep_user_passwd": "passwd",
            "verbose": False,
            "dry_run": False,
        }

        # join needs start
        if is_member_of_group(self.server):
            leave(self.server, **options)

        # reconnect the server.
        self.server.connect()

        self.assertTrue(start(self.server, **options))

        # reconnect the server.
        self.server.connect()

        # health
        health(self.server, **options)

        # reconnect the server.
        self.server.connect()

        leave(self.server, **options)

        # reconnect the server.
        self.server.connect()

        member_state_qry = ("SELECT MEMBER_STATE FROM {0} as m JOIN {1} "
                            "as s on m.MEMBER_ID = s.MEMBER_ID"
                            "".format(REP_GROUP_MEMBERS_TABLE,
                                      REP_MEMBER_STATS_TABLE))
        frozen_queries = {
            member_state_qry: [['ONLINE', ], ],
        }
        mock_server = get_mock_server(self.server, queries=frozen_queries)

        # Join with defaults ("dry_run": True) and required password
        options = {
            "dry_run": True,
            "replication_user": "replicator_user@'%'",
            "rep_user_passwd": "passwd",
        }
        join(self.server, mock_server, **options)

        # reconnect the server.
        self.server.connect()

        # Join with no defaults ("dry_run": True)
        options = {
            "group_name": None,
            "gr_host": "{0}:{1}".format(self.server.host, self.server.port),
            "replication_user": "replicator_user@'%'",
            "rep_user_passwd": "passwd",
            "verbose": False,
            "dry_run": True,
        }
        # leave the group
        if is_member_of_group(self.server):
            leave(self.server, **options)

        self.assertFalse(join(self.server, mock_server, **options))

        # reconnect the server.
        self.server.connect()

        self.assertFalse(leave(self.server, **options))

        # reconnect the server.
        self.server.connect()

    def test_re_join(self):
        """Tests join method over actively replicating server
        """
        # Fill the options
        options = {
            "group_name": None,
            "gr_host": self.server.host,
            "replication_user": "replicator_user@'%'",
            "rep_user_passwd": "passwd",
            "verbose": False,
            "dry_run": False,
        }

        # test start
        leave(self.server, **options)

        # reconnect the server.
        self.server.connect()

        self.assertTrue(start(self.server, **options))

        # reconnect the server.
        self.server.connect()

        # test health
        health(self.server, **options)

        # reconnect the server.
        self.server.connect()

        options["dry_run"] = True
        # test join
        mock_server = get_mock_server(self.server)
        # Trying to add server to a group while is already a member of a group
        # expect GadgetError: Query failed: is already a member of a GR group.
        with self.assertRaises(GadgetError) as test_raises:
            join(self.server, mock_server, **options)
        exception = test_raises.exception
        self.assertTrue("is already a member" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        self.server.connect()

        self.assertTrue(leave(self.server, **options))

        # reconnect the server.
        self.server.connect()

    def test_health_with_not_a_member(self):
        """Test the commands that requires a GR member or None value for a
        required server connection information.
        """
        # Fill the options
        options = {}

        # expect GadgetError: Query failed: no server was given
        with self.assertRaises(GadgetError) as test_raises:
            health(None, **options)
        exception = test_raises.exception
        self.assertTrue("No server was given" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # expect GadgetError: Query failed: not a member of a Group
        with self.assertRaises(GadgetError) as test_raises:
            health(self.mock_no_member, **options)
        exception = test_raises.exception
        self.assertTrue("not a member of a GR group" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # Fill the options
        options = {
            "group_name": None,
            "replication_user": None,
            "rep_user_passwd": "passwd",
        }

        # expect GadgetError: Query failed: No server was given
        with self.assertRaises(GadgetError) as test_raises:
            start(None, **options)
        exception = test_raises.exception
        self.assertTrue("No server was given" in exception.errmsg,
                        "The exception message was not the expected: {0}"
                        "".format(exception.errmsg))

    def test_join_with_none_values(self):
        """Test the commands that requires a GR member or None value for a
        required server connection information.
        """
        # Fill the options
        options = {
            "group_name": None,
            "replication_user": None,
            "rep_user_passwd": "passwd",
            "gr_host": None,
        }

        # expect GadgetError: Query failed: No peer server was given
        with self.assertRaises(GadgetError) as test_raises:
            join(None, None, **options)
        exception = test_raises.exception
        self.assertTrue("No server was given" in exception.errmsg,
                        "The exception message was not the expected: {0}"
                        "".format(exception.errmsg))

        # expect GadgetError: Query failed: No peer server was given
        with self.assertRaises(GadgetError) as test_raises:
            join(self.server, None, **options)
        exception = test_raises.exception
        self.assertTrue("No peer server provided" in exception.errmsg,
                        "The exception message was not the expected: {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        self.server.connect()

        # expect GadgetError: Query failed: not a member of a Group
        with self.assertRaises(GadgetError) as test_raises:
            join(self.server, self.mock_no_member, **options)
        exception = test_raises.exception
        self.assertTrue("not a member of a GR group" in exception.errmsg,
                        "The exception message was not the expected: {0}"
                        "".format(exception.errmsg))

        # reconnect the server.
        self.server.connect()

    def test_leave_with_none_values(self):
        """Test the commands that requires a GR member or None value for a
        required server connection information.
        """
        # Fill the options
        options = {}
        # expect GadgetError: Query failed: not a member of a Group
        with self.assertRaises(GadgetError) as test_raises:
            leave(None, **options)
        exception = test_raises.exception
        self.assertTrue("No server was given" in exception.errmsg,
                        "The exception message was not the expected: {0}"
                        "".format(exception.errmsg))

        # expect GadgetError: Query failed: not a member of a Group
        with self.assertRaises(GadgetError) as test_raises:
            leave(self.mock_no_member, **options)
        exception = test_raises.exception
        self.assertTrue("not a member of a Group" in exception.errmsg,
                        "The exception message was not the expected: {0}"
                        "".format(exception.errmsg))

    def test_resolve_gr_local_address(self):
        """Tests resolve_gr_local_address method.
        """
        # Use server host and port
        gr_host = ""
        server_host = self.server.host
        server_port = get_free_random_port()
        self.assertEqual(resolve_gr_local_address(gr_host, server_host,
                                                  server_port),
                         (self.server.host, repr(server_port + 10000)))

        # Use host from gr_host
        gr_host = "host4321"
        server_port = get_free_random_port()
        self.assertEqual(resolve_gr_local_address(gr_host, server_host,
                                                  server_port),
                         ("host4321", repr(server_port + 10000)))

        # Use host from gr_host
        gr_host = "127.0.0.1:"
        server_port = get_free_random_port()
        self.assertEqual(resolve_gr_local_address(gr_host, server_host,
                                                  server_port),
                         ("127.0.0.1", repr(server_port + 10000)))

        # Use given IPv6 host and port from gr_host
        gr_host = "[1:2:3:4:5:6:7:8]:1234"
        self.assertEqual(resolve_gr_local_address(gr_host, server_host,
                                                  server_port),
                         ("1:2:3:4:5:6:7:8", "1234"))

        # Use given IPv6 host and port from gr_host
        server_port = get_free_random_port()
        gr_host = "E3D7::51F4:9BC8:{0}".format(server_port)
        self.assertEqual(resolve_gr_local_address(gr_host, server_host,
                                                  server_port),
                         ("E3D7::51F4:9BC8", repr(server_port)))

        # Use given port from gr_host
        gr_host = "1234"
        server_port = get_free_random_port()
        self.assertEqual(resolve_gr_local_address(gr_host, server_host,
                                                  server_port),
                         (self.server.host, "1234"))

        # Given port on gr_host is out of range, generate a new one instead
        gr_host = "65536"
        self.assertIsInstance(
            int(resolve_gr_local_address(gr_host, server_host,
                                         server_port)[1]), int)

        # Use given port from gr_host
        gr_host = ":1234"
        self.assertEqual(resolve_gr_local_address(gr_host, server_host,
                                                  server_port),
                         (self.server.host, "1234"))

        # Use given IPv6 host from gr_host and generate random port.
        gr_host = "E3D7::51F4:9BC8"
        self.assertEqual(resolve_gr_local_address(gr_host, server_host,
                                                  server_port)[0],
                         "E3D7::51F4:9BC8")
        self.assertIsInstance(
            int(resolve_gr_local_address(gr_host, server_host,
                                         server_port)[1]), int)
