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
Unit tests for mysql_gadgets.common.group_replication module.
"""
import unittest

from unit_tests.utils import GadgetsTestCase, SERVER_CNX_OPT

from mysql_gadgets.common.server import Server

from mysql_gadgets.common.group_replication import (check_server_requirements,
                                                    install_plugin,
                                                    get_gr_config_vars,
                                                    get_req_dict,
                                                    get_rpl_usr,
                                                    GR_GROUP_NAME,
                                                    GR_GROUP_SEEDS,
                                                    GR_IP_WHITELIST,
                                                    GR_LOCAL_ADDRESS,
                                                    GR_PLUGIN_NAME,
                                                    GR_RECOVERY_USE_SSL,
                                                    GR_SINGLE_PRIMARY_MODE,
                                                    GR_SSL_MODE,
                                                    GR_SSL_DISABLED,
                                                    GR_SSL_REQUIRED)


class TestGRPlugin(GadgetsTestCase):
    """This test class exercises the gr_plugin module at
    mysql_gadgets.common.server.
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
        req_dict = get_req_dict(self.server, None, None, None)
        try:
            check_server_requirements(self.server, req_dict, None,
                                      verbose=False, dry_run=False,
                                      skip_schema_checks=False, update=False)
        except:
            raise unittest.SkipTest("Provided server must fulfill the GR "
                                    "plugin requirements.")

    def tearDown(self):
        """ Disconnect base server (for all tests).
        """
        self.server.exec_query("drop user if exists 'rpl_user'")
        self.server.exec_query("drop user if exists 'replicator'")
        self.server.disconnect()

    def test_install_gr_plugin(self):
        """Test the install_gr_plugin method"""
        if self.server.is_plugin_installed(GR_PLUGIN_NAME):
            self.server.exec_query("UNINSTALL PLUGIN group_replication")
        install_plugin(self.server, dry_run=True)
        self.assertFalse(self.server.is_plugin_installed(GR_PLUGIN_NAME),
                         "GR plugin was expected to be loaded")
        install_plugin(self.server)
        self.assertTrue(self.server.is_plugin_installed(GR_PLUGIN_NAME),
                        "GR plugin was expected to be installed")

    def test_get_rpl_usr(self):
        """Tests get_rpl_usr method"""
        options = {"rep_user_passwd": "rpl_pass"}
        rpl_user_dict = {
            'host': '%',
            'recovery_user': 'rpl_user',
            'rep_user_passwd': 'rpl_pass',
            'replication_user': "rpl_user@'%'",
            'ssl_mode': GR_SSL_REQUIRED,
        }
        self.assertDictEqual(get_rpl_usr(options), rpl_user_dict)
        options = {
            "rep_user_passwd": "my_password",
            "replication_user": "replicator@oracle.com",
            'ssl_mode': GR_SSL_DISABLED,
        }
        rpl_user_dict = {
            'host': 'oracle.com',
            "recovery_user": "replicator",
            'rep_user_passwd': 'my_password',
            'replication_user': "replicator@'oracle.com'",
            'ssl_mode': GR_SSL_DISABLED,
        }
        self.assertDictEqual(get_rpl_usr(options), rpl_user_dict)

    def test_get_gr_config_vars(self):
        """Test get_gr_config_vars method"""
        options = {
            "group_name": None,
        }
        local_address = "localhost:1234"
        gr_config_vars = {
            GR_LOCAL_ADDRESS: "'{0}'".format(local_address),
            GR_SINGLE_PRIMARY_MODE: None,
            GR_GROUP_NAME: None,
            GR_GROUP_SEEDS: None,
            GR_IP_WHITELIST: None,
            GR_RECOVERY_USE_SSL: "'ON'",
            GR_SSL_MODE: "'REQUIRED'"
        }
        self.assertDictEqual(get_gr_config_vars(local_address, options),
                             gr_config_vars)

        options = {
            "group_name": "18e76fd4-2d91-11e6-9b7e-507b9d87510a",
        }

        gr_config_vars = {
            GR_GROUP_NAME: "'18e76fd4-2d91-11e6-9b7e-507b9d87510a'",
            GR_LOCAL_ADDRESS: "'{0}'".format(local_address),
            GR_SINGLE_PRIMARY_MODE: None,
            GR_GROUP_SEEDS: None,
            GR_IP_WHITELIST: None,
            GR_RECOVERY_USE_SSL: "'ON'",
            GR_SSL_MODE: "'REQUIRED'"
        }
        self.assertDictEqual(get_gr_config_vars(local_address, options),
                             gr_config_vars)

    def test_check_server_requirements(self):
        """Test check_server_requirements method
        """
        rpl_settings = {
            "recovery_user": "replicator",
            'rep_user_passwd': 'my_password',
            'replication_user': "replicator@'oracle.com'",
            "host": '%',
        }
        req_dict = get_req_dict(self.server, rpl_settings["replication_user"])
        verbose = False
        dry_run = True
        check_server_requirements(self.server, req_dict, rpl_settings, verbose,
                                  dry_run)

        rpl_settings = {
            "recovery_user": "replicator",
            'rep_user_passwd': 'my_password',
            'replication_user': "replicator@'%'",
            "host": '%',
        }
        req_dict = get_req_dict(self.server, rpl_settings["replication_user"])
        dry_run = True
        check_server_requirements(self.server, req_dict, rpl_settings, verbose,
                                  dry_run)

        check_server_requirements(self.server, req_dict, rpl_settings)
