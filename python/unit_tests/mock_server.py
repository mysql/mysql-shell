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
mock server module
"""
import logging

from mysql_gadgets.common import server

from mysql_gadgets.common.group_replication import (GR_LOCAL_ADDRESS,
                                                    GR_PLUGIN_NAME,
                                                    REP_CONN_STATUS_TABLE,
                                                    REP_GROUP_MEMBERS_TABLE)

ORIGINAL_GET_SERVER = server.get_server

# Get common logger
_LOGGER = logging.getLogger(__name__)

FROZEN_QUERIES = {
    "SELECT @@GLOBAL.{0}".format(GR_LOCAL_ADDRESS):
        [["localhost:3307"]],
    "SELECT @@GLOBAL.{0}".format("server_id"):
        [["7"]],
    ("SELECT GROUP_NAME FROM {0} WHERE "
     "CHANNEL_NAME='group_replication_applier'".format(REP_CONN_STATUS_TABLE)):
        [["b7286041-3016-11e6-ba52-507b9d87510a"]],
}
FROZEN_VARIABLES = {
    "server_id": "7",
}


class MockServer(object):
    """Mock server class
    """
    def __init__(self, server1, queries=None, variables=None,
                 plugin_installed=None):
        if queries is None:
            queries = {}
        if variables is None:
            variables = {}
        self.server = server1

        self.user = self.server.user
        self.host = self.server.host
        self.port = self.server.port
        self.get_version = self.server.get_version
        self._version = self.server._version  # pylint: disable=W0212
        self.grant_tables_enabled = self.server.grant_tables_enabled
        self.binlog_enabled = self.server.binlog_enabled
        self.toggle_binlog = self.server.toggle_binlog

        self.frozen_queries = FROZEN_QUERIES.copy()
        qry_key = ("select MEMBER_HOST, MEMBER_PORT from {0}"
                   "".format(REP_GROUP_MEMBERS_TABLE))
        self.frozen_queries[qry_key] = [[self.server.host,
                                         self.server.port + 1]]
        self.frozen_queries.update(queries)

        self.frozen_variables = FROZEN_VARIABLES.copy()
        self.frozen_variables.update(variables)

        self.plugin_installed = plugin_installed
        if self.plugin_installed is None:
            self.plugin_installed = {GR_PLUGIN_NAME: True}

    def exec_query(self, query, options=None, exec_timeout=0,
                   query_to_log=None):
        """Simulates Query execution"""
        return self.frozen_queries.get(query,
                                       self.server.exec_query(query, options,
                                                              exec_timeout,
                                                              query_to_log))

    def disconnect(self):
        """Simulates server disconnect method.

        Mock server must not be disconnected otherwise will cause the original
        server to be disconnected when could be still need to be connected.
        """
        pass

    def is_plugin_installed(self, plugin_name, is_active=False,
                            silence_warnings=False):
        """mocked is_plugin_installed method"""
        _LOGGER.debug("silence_warnings: %s, is_active: %s", silence_warnings,
                      is_active)
        status = self.plugin_installed[plugin_name]
        return status

    def get_connection_values(self):
        """mocked get_connection_values method"""
        return self.server.get_connection_values()

    def select_variable(self, var_name, var_type=None):
        """mocked select_variable method"""
        if var_name in self.frozen_variables.keys():
            return self.frozen_variables[var_name]
        else:
            return self.server.select_variable(var_name, var_type)

    def __str__(self):
        """mocked string representation of the class Server

        :return: representation the server with information of the host
                 and port.
        :rtype:  string
        """
        return self.server.__str__()


def mock_get_server(server_info, ssl_dict=None, connect=True):
    """The insider method to mock Server.get_server method"""
    if isinstance(server_info, MockServer):
        mocked_server = server_info
    else:
        mocked_server = ORIGINAL_GET_SERVER(server_info, ssl_dict, connect)
    return mocked_server

# Monkey patch for get_server; returns mock_server when it is given as
# connection information.
server.get_server = mock_get_server


def get_mock_server(server_base, **kwargs):
    """Creates a MockServer object based on the given server instance"""
    return MockServer(server_base, **kwargs)
