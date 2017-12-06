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
Unit tests for mysql_gadgets.common.req_checker module.
"""
import os
import unittest

from mysql_gadgets.common.group_replication import GR_REQUIRED_CONFIG
from mysql_gadgets.common.req_checker import (ONE_OF,
                                              RequirementChecker,)
from mysql_gadgets.common import server
from unit_tests.utils import (GadgetsTestCase,
                              SERVER_CNX_OPT)

SERVER_VARIABLESS = {
    "port": {ONE_OF: ("1001",)},
    "basedir": {ONE_OF: ("c:/mysql",)},
    "datadir": {ONE_OF: ("c:/mysql/datadir",)},

    "bind-address": {ONE_OF: ("127.0.0.1",)},
    "log_slave_updates": {ONE_OF: ("1",)},
    "enforce_gtid_consistency": {ONE_OF: ("ON",)},

    "master_info_repository": {ONE_OF: ("TABLE",)},
    "relay_log_info_repository": {ONE_OF: ("FILE",)},
    "transaction_write_set_extraction": {ONE_OF: ("MURMUR32",)},
}


class TestReqChecker(GadgetsTestCase):
    """Unit Test Class for the mysql_gadgets.common.req_checker module.
    This test only test the config parser use.
    """

    @property
    def num_servers_required(self):
        """Property defining the number of servers required by the test.
        """
        return 1

    def setUp(self):
        """setUp method"""
        self.maxDiff = None
        self.server_cnx = {'conn_info': self.options[SERVER_CNX_OPT][0]}
        self.server = server.Server(self.server_cnx)
        self.server.connect()
        self.req_check = RequirementChecker()
        self.option_file = os.path.normpath(
            os.path.join(__file__, "..", "std_data", "option_files", 'my.cnf'))

    def test_check_config_settings_default(self):
        """Tests check_config_settings default option"""
        var_values = GR_REQUIRED_CONFIG
        results = self.req_check.check_config_settings(var_values,
                                                       self.option_file,
                                                       self.server)
        self.assertFalse(results["pass"])
        expected_result = {
            'master_info_repository': (False, 'TABLE', 'FILE'),
            'binlog_checksum': (False, 'NONE', '<not set>'),
            'binlog_format': (False, 'ROW', '<not set>'),
            'enforce_gtid_consistency': (False, 'ON', '<not set>'),
            'gtid_mode': (False, 'ON', '<not set>'),
            'log_bin': (False, '1', '<not set>'),
            'log_slave_updates': (False, 'ON', '<not set>'),
            'relay_log_info_repository': (False, 'TABLE', '<not set>'),
            'report_port': (False, str(self.server.port), '<not set>'),
            'transaction_write_set_extraction': (False, 'XXHASH64',
                                                 '<not set>'),
            'pass': False
        }
        if 'report_port' not in results.keys():
            expected_result.pop('report_port')
        self.assertDictEqual(results, expected_result)

    def test_add_store_connection_option_mix(self):
        """Tests check_config_settings mix options"""
        var_values = SERVER_VARIABLESS.copy()
        results = self.req_check.check_config_settings(var_values,
                                                       self.option_file,
                                                       self.server)
        self.assertFalse(results["pass"])
        expected_result = {
            'basedir': (False, 'c:/mysql', '/usr'),
            'bind_address': (True, '127.0.0.1', '127.0.0.1'),
            'datadir': (False, 'c:/mysql/datadir', '/var/lib/mysql'),
            'master_info_repository': (False, 'TABLE', 'FILE'),

            'pass': False,
            'port': (True, '1001', '1001'),
            'enforce_gtid_consistency': (False, 'ON', '<not set>'),
            'log_slave_updates': (False, '1', '<not set>'),
            'relay_log_info_repository': (False, 'FILE', '<not set>'),
            'transaction_write_set_extraction': (False, 'MURMUR32',
                                                 '<not set>')
        }

        self.assertDictEqual(results, expected_result)

if __name__ == "__main__":
    unittest.main()
