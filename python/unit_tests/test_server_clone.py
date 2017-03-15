# -*- coding: utf-8 -*-
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
This file contains unit unittests for the the mysql_gadgets.command clone
module.
"""
import os
import tempfile

import mysql_gadgets.command.clone
from mysql_gadgets import exceptions
from mysql_gadgets.adapters import MYSQL_DEST, MYSQL_SOURCE
from mysql_gadgets.common import server
from unit_tests.utils import GadgetsTestCase, SERVER_CNX_OPT


class TestClone(GadgetsTestCase):
    """Class to test mysql_gadgets.command.clone module
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

    def tearDown(self):
        """ Disconnect from servers and drop created databases (for all tests).
        """

        self.source.disconnect()
        self.destination.disconnect()

    def test_clone_server_errors(self):
        """test clone_server function error conditions"""

        # Passing invalid connection dictionaries should yield an error
        # Missing user on source
        conn_dict = {MYSQL_SOURCE: {"host": "localhost", "port": 3304},
                     MYSQL_DEST: self.options[SERVER_CNX_OPT][1]}
        with self.assertRaises(exceptions.GadgetError):
            mysql_gadgets.command.clone.clone_server(conn_dict, "adapter")
        # Missing user on destination
        conn_dict = {MYSQL_SOURCE: self.options[SERVER_CNX_OPT][0],
                     MYSQL_DEST: {"host": "localhost", "port": 3304}}
        with self.assertRaises(exceptions.GadgetError):
            mysql_gadgets.command.clone.clone_server(conn_dict, "adapter")

        # Using valid connection dictionaries to servers that are not running
        # should also yield an error.
        conn_dict = {MYSQL_SOURCE: {"host": "localhost", "port": 999989,
                                    "user": "root"},
                     MYSQL_DEST: self.options[SERVER_CNX_OPT][1]}
        with self.assertRaises(exceptions.GadgetError):
            mysql_gadgets.command.clone.clone_server(conn_dict, "adapter")
        conn_dict = {MYSQL_SOURCE: self.options[SERVER_CNX_OPT][0],
                     MYSQL_DEST: {"host": "localhost", "port": 999989,
                                  "user": "root"}}
        with self.assertRaises(exceptions.GadgetError):
            mysql_gadgets.command.clone.clone_server(conn_dict, "adapter")

        # Using the same server for both source and destination should
        # yield an error
        conn_dict = {MYSQL_SOURCE: self.options[SERVER_CNX_OPT][0],
                     MYSQL_DEST: self.options[SERVER_CNX_OPT][0]}
        with self.assertRaises(exceptions.GadgetError):
            mysql_gadgets.command.clone.clone_server(conn_dict, "adapter")

        # Having two valid servers but passing an invalid adapter should
        # raise an error.
        conn_dict = {MYSQL_SOURCE: self.options[SERVER_CNX_OPT][0],
                     MYSQL_DEST: self.options[SERVER_CNX_OPT][1]}
        with self.assertRaises(exceptions.GadgetError):
            mysql_gadgets.command.clone.clone_server(
                conn_dict, "invalid_adapter")

        # Using the image based clone with an invalid adapter should raise
        # an error.
        with self.assertRaises(exceptions.GadgetError):
            mysql_gadgets.command.clone.clone_server(conn_dict,
                                                     "invalid_adapter",
                                                     "filename")

        # using a valid adapter and image based cloning but passing a file that
        # already exists should also raise an error.
        fd, fpath = tempfile.mkstemp()
        os.close(fd)
        try:
            with self.assertRaises(exceptions.GadgetError):
                mysql_gadgets.command.clone.clone_server(conn_dict,
                                                         "MySQLDump", fpath)
        finally:
            os.remove(fpath)
