# -*- coding: utf-8 -*-
#
# Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
This file contains unit unittests for the the MySQLDump adapter.
"""
import os
import unittest

import mysql_gadgets.command.clone
from mysql_gadgets import exceptions
from mysql_gadgets.adapters import MYSQL_DEST, MYSQL_SOURCE
from mysql_gadgets.common import server
from mysql_gadgets.common.tools import get_tool_path
from unit_tests.utils import GadgetsTestCase, SERVER_CNX_OPT


class TestMySQLDumpAdapter(GadgetsTestCase):
    """Class to test MySQLDump adapter
    """
    @property
    def num_servers_required(self):
        """Property defining the number of servers required by the test.
        """
        return 2

    def setUp(self):
        """ Setup server connections (for all tests) .
        """
        self.source_cnx = {'conn_info': self.options[SERVER_CNX_OPT][0]}
        self.destination_cnx = {'conn_info': self.options[SERVER_CNX_OPT][1]}
        self.source = server.Server(self.source_cnx)
        self.destination = server.Server(self.destination_cnx)
        self.source.connect()
        self.destination.connect()
        try:
            self.source_gtids_enabled = self.source.supports_gtid()
        except exceptions.GadgetServerError:
            self.source_gtids_enabled = False
        try:
            self.destination_gtids_enabled = self.destination.supports_gtid()
        except exceptions.GadgetServerError:
            self.destination_gtids_enabled = False

        if self.destination_gtids_enabled:
            self.destination_has_no_gtid_executed = not \
                self.destination.get_gtid_executed(skip_gtid_check=False)
        else:
            self.destination_has_no_gtid_executed = False

        self.mysqldump_exec = get_tool_path(None, "mysqldump",
                                            search_path=True, required=False)
        self.mysql_exec = get_tool_path(None, "mysql", search_path=True,
                                        required=False)

        # Search for mysqldump only on path (ignore MySQL default paths).
        self.mysqldump_exec_in_defaults = get_tool_path(
            None, "mysqldump", search_path=False, required=False)

        # setup source server with a sample database and users
        data_sql_path = os.path.normpath(
            os.path.join(__file__, "..", "std_data", "basic_data.sql"))
        self.source.read_and_exec_sql(data_sql_path, verbose=True)

        # Create user without super permission on both source and destination
        self.source.exec_query("create user no_super@'%'")
        self.source.exec_query("GRANT ALL on *.* to no_super@'%'")
        self.source.exec_query("REVOKE SUPER on *.* from no_super@'%'")
        self.destination.exec_query("create user no_super@'%'")
        self.destination.exec_query("GRANT ALL on *.* to no_super@'%'")
        self.destination.exec_query("REVOKE SUPER on *.* from no_super@'%'")
        if self.destination_has_no_gtid_executed:
            self.destination.exec_query("RESET MASTER")

    def tearDown(self):
        """ Disconnect from servers and drop created databases (for all tests).
        """
        # Drop added databases/users on source if they exist
        self.source.exec_query("drop database if exists util_test")
        self.source.exec_query("drop user if exists joe_wildcard")
        self.source.exec_query("drop user if exists 'joe'@'user'")
        self.source.exec_query("drop user if exists no_super")

        # Drop added databases/users on destination if they exist
        self.destination.exec_query("drop database if exists util_test")
        self.destination.exec_query("drop user if exists joe_wildcard")
        self.destination.exec_query("drop user if exists 'joe'@'user'")
        self.destination.exec_query("drop user if exists no_super")

        # If destination GTID set was empty at the beginning of the test
        # reset it again
        if self.destination_has_no_gtid_executed:
            self.destination.exec_query("RESET MASTER")

        self.source.disconnect()
        self.destination.disconnect()

    def test_clone_server_stream(self):
        """Test stream cloning functionality of MySQLDump adapter.
        Requires source and destination servers to have the same GTID mode.
        Also if GTID is enabled it requires the destination server to
        have an empty GTID_EXECUTED set.
        """
        if self.source_gtids_enabled != self.destination_gtids_enabled:
            raise unittest.SkipTest("Provided servers must have the same "
                                    "GTID mode.")

        if self.destination_gtids_enabled and \
                not self.destination_has_no_gtid_executed:
            raise unittest.SkipTest("Second server provided as argument must "
                                    "have an empty gtid_executed set.")

        if not self.mysqldump_exec:
            raise unittest.SkipTest("Test requires mysqldump tool to be on "
                                    "the $PATH")
        if not self.mysql_exec:
            raise unittest.SkipTest("Test requires mysql client tool to be on "
                                    "the $PATH")

        conn_dict = {MYSQL_SOURCE: self.options[SERVER_CNX_OPT][0],
                     MYSQL_DEST: self.options[SERVER_CNX_OPT][1]}

        # Only execute this test if mysqldump is not found in default paths.
        if not self.mysqldump_exec_in_defaults:
            old_path = os.environ["PATH"]
            # erase path and hide mysqldump
            try:
                os.environ["PATH"] = ""
                # it must fail since it cannot find the tool
                with self.assertRaises(exceptions.GadgetError) as cm:
                    mysql_gadgets.command.clone.clone_server(conn_dict,
                                                             "MySQLDump")
                self.assertIn("Could not find mysqldump executable",
                              str(cm.exception))
            finally:
                # restore PATH
                os.environ["PATH"] = old_path

        # Create connection dicts for users without permissions
        source_no_super = self.source.get_connection_values()
        source_no_super["user"] = "no_super"
        source_no_super["passwd"] = ""
        dest_no_super = self.destination.get_connection_values()
        dest_no_super["user"] = "no_super"
        dest_no_super["passwd"] = ""

        no_perm_conn_dict = {MYSQL_SOURCE: source_no_super,
                             MYSQL_DEST: self.options[SERVER_CNX_OPT][1]}

        # It must fail when source user does't have super permission.
        with self.assertRaises(exceptions.GadgetError) as cm:
            mysql_gadgets.command.clone.clone_server(no_perm_conn_dict,
                                                     "MySQLDump")
        self.assertEqual("SUPER privilege is required for the MySQL user "
                         "'no_super' on the source server.",
                         str(cm.exception))

        # it must fail when destination user doesn't have super permission.
        if self.destination_gtids_enabled:
            no_perm_conn_dict = {MYSQL_SOURCE: self.options[SERVER_CNX_OPT][0],
                                 MYSQL_DEST: dest_no_super}
            with self.assertRaises(exceptions.GadgetError) as cm:
                mysql_gadgets.command.clone.clone_server(no_perm_conn_dict,
                                                         "MySQLDump")
            self.assertEqual("SUPER privilege is required for the MySQL user "
                             "'no_super' on the destination server.",
                             str(cm.exception))

        # test stream cloning using mysqldump.
        mysql_gadgets.command.clone.clone_server(conn_dict, "MySQLDump")
        # Check that database util_test exists on destination.
        self.assertIn(("util_test",), self.destination.get_all_databases())
        # Check that all tables from util_test db were copied.
        self.assertEqual(
            self.source.exec_query(
                "SELECT COUNT(*) FROM information_schema.tables "
                "WHERE table_schema = 'util_test'"),
            self.destination.exec_query(
                "SELECT COUNT(*) FROM information_schema.tables "
                "WHERE table_schema = 'util_test'"))

        # Check that all views from util_test db were copied.
        self.assertEqual(
            self.source.exec_query(
                "SELECT COUNT(*) FROM information_schema.views "
                "WHERE table_schema = 'util_test'"),
            self.destination.exec_query(
                "SELECT COUNT(*) FROM information_schema.views "
                "WHERE table_schema = 'util_test'"))

        # Check that all triggers from util_test db were copied.
        self.assertEqual(
            self.source.exec_query(
                "SELECT COUNT(*) FROM information_schema.triggers "
                "WHERE trigger_schema = 'util_test'"),
            self.destination.exec_query(
                "SELECT COUNT(*) FROM information_schema.triggers "
                "WHERE trigger_schema = 'util_test'"))

        # Check that all events from util_test db were copied.
        self.assertEqual(
            self.source.exec_query(
                "SELECT COUNT(*) FROM information_schema.events "
                "WHERE event_schema = 'util_test'"),
            self.destination.exec_query(
                "SELECT COUNT(*) FROM information_schema.events "
                "WHERE event_schema = 'util_test'"))

        # Check that all routines from util_test db were copied.
        self.assertEqual(
            self.source.exec_query(
                "SELECT COUNT(*) FROM information_schema.routines "
                "WHERE routine_schema = 'util_test'"),
            self.destination.exec_query(
                "SELECT COUNT(*) FROM information_schema.routines "
                "WHERE routine_schema = 'util_test'"))

        # Check all users were copied
        self.assertEqual(
            self.source.exec_query("SELECT COUNT(*) FROM mysql.user"),
            self.destination.exec_query("SELECT COUNT(*) FROM mysql.user"))

    def test_clone_server_image(self):
        """Test image cloning functionality of MySQLDump adapter.
        Requires source and destination servers to have the same GTID mode.
        Also if GTID is enabled it requires the destination server to
        have an empty GTID_EXECUTED set.
        """
        if self.source_gtids_enabled != self.destination_gtids_enabled:
            raise unittest.SkipTest("Provided servers must have the same "
                                    "GTID mode.")

        if self.destination_gtids_enabled and \
                not self.destination_has_no_gtid_executed:
            raise unittest.SkipTest("Second server provided as argument must "
                                    "have an empty gtid_executed set.")

        if not self.mysqldump_exec:
            raise unittest.SkipTest("Test requires mysqldump tool to be on "
                                    "the $PATH")
        if not self.mysql_exec:
            raise unittest.SkipTest("Test requires mysql client tool to be on "
                                    "the $PATH")

        conn_dict = {MYSQL_SOURCE: self.options[SERVER_CNX_OPT][0],
                     MYSQL_DEST: self.options[SERVER_CNX_OPT][1]}

        fpath = os.path.join(os.path.dirname(__file__), "image_file")

        # Only execute this test if mysqldump is not found in default paths.
        old_path = os.environ["PATH"]
        if not self.mysqldump_exec_in_defaults:
            # erase path and hide mysqldump
            try:
                os.environ["PATH"] = ""
                # it must fail since it cannot find the tool
                with self.assertRaises(exceptions.GadgetError) as cm:
                    mysql_gadgets.command.clone.clone_server(conn_dict,
                                                             "MySQLDump",
                                                             fpath)
                self.assertIn("Could not find mysqldump executable",
                              str(cm.exception))
            finally:
                # restore PATH
                os.environ["PATH"] = old_path

        # Create connection dicts for users without permissions
        source_no_super = self.source.get_connection_values()
        source_no_super["user"] = "no_super"
        source_no_super["passwd"] = ""
        dest_no_super = self.destination.get_connection_values()
        dest_no_super["user"] = "no_super"
        dest_no_super["passwd"] = ""

        no_perm_conn_dict = {MYSQL_SOURCE: source_no_super,
                             MYSQL_DEST: self.options[SERVER_CNX_OPT][1]}

        # It must fail when source user does't have super permission.
        with self.assertRaises(exceptions.GadgetError) as cm:
            mysql_gadgets.command.clone.clone_server(no_perm_conn_dict,
                                                     "MySQLDump", fpath)
        self.assertEqual("SUPER privilege is required for the MySQL user "
                         "'no_super' on the source server.",
                         str(cm.exception))

        # it must fail when destination user doesn't have super permission.
        if self.destination_gtids_enabled:
            no_perm_conn_dict = {MYSQL_SOURCE: self.options[SERVER_CNX_OPT][0],
                                 MYSQL_DEST: dest_no_super}
            with self.assertRaises(exceptions.GadgetError) as cm:
                mysql_gadgets.command.clone.clone_server(no_perm_conn_dict,
                                                         "MySQLDump", fpath)
            self.assertEqual("SUPER privilege is required for the MySQL user "
                             "'no_super' on the destination server.",
                             str(cm.exception))
        # test image cloning using mysqldump.
        try:
            mysql_gadgets.command.clone.clone_server(conn_dict, "MySQLDump",
                                                     fpath)

            # GTID executed must be the same on both servers.
            self.assertEqual(self.source.get_gtid_executed(),
                             self.destination.get_gtid_executed())
            # Check that database util_test exists on destination.
            self.assertIn(("util_test",), self.destination.get_all_databases())
            # Check that all tables from util_test db were copied.
            self.assertEqual(
                self.source.exec_query(
                    "SELECT COUNT(*) FROM information_schema.tables "
                    "WHERE table_schema = 'util_test'"),
                self.destination.exec_query(
                    "SELECT COUNT(*) FROM information_schema.tables "
                    "WHERE table_schema = 'util_test'"))

            # Check that all views from util_test db were copied.
            self.assertEqual(
                self.source.exec_query(
                    "SELECT COUNT(*) FROM information_schema.views "
                    "WHERE table_schema = 'util_test'"),
                self.destination.exec_query(
                    "SELECT COUNT(*) FROM information_schema.views "
                    "WHERE table_schema = 'util_test'"))

            # Check that all triggers from util_test db were copied.
            self.assertEqual(
                self.source.exec_query(
                    "SELECT COUNT(*) FROM information_schema.triggers "
                    "WHERE trigger_schema = 'util_test'"),
                self.destination.exec_query(
                    "SELECT COUNT(*) FROM information_schema.triggers "
                    "WHERE trigger_schema = 'util_test'"))

            # Check that all events from util_test db were copied.
            self.assertEqual(
                self.source.exec_query(
                    "SELECT COUNT(*) FROM information_schema.events "
                    "WHERE event_schema = 'util_test'"),
                self.destination.exec_query(
                    "SELECT COUNT(*) FROM information_schema.events "
                    "WHERE event_schema = 'util_test'"))

            # Check that all routines from util_test db were copied.
            self.assertEqual(
                self.source.exec_query(
                    "SELECT COUNT(*) FROM information_schema.routines "
                    "WHERE routine_schema = 'util_test'"),
                self.destination.exec_query(
                    "SELECT COUNT(*) FROM information_schema.routines "
                    "WHERE routine_schema = 'util_test'"))

            # Check all users were copied
            self.assertEqual(
                self.source.exec_query("SELECT COUNT(*) FROM mysql.user"),
                self.destination.exec_query("SELECT COUNT(*) FROM mysql.user")
            )
        finally:
            os.remove(fpath)
