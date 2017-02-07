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

import os
import shutil
from collections import OrderedDict
from unit_tests.utils import (GadgetsTestCase, SERVER_CNX_OPT,
                              skip_if_not_GR_approved,)

from unit_tests.mock_server import get_mock_server

from mysql_gadgets.common.config_parser import MySQLOptionsParser
from mysql_gadgets.common.server import Server
from mysql_gadgets.common.group_replication import (
    _format_table_results,
    _print_check_unique_id_results,
    _print_check_server_version_results,
    CONFIG_SETTINGS,
    DYNAMIC_SERVER_VARS,
    get_gr_configs_from_instance,
    get_gr_local_address_from,
    get_gr_members,
    get_gr_name_from_peer,
    get_req_dict,
    get_rpl_usr,
    GR_LOCAL_ADDRESS,
    OPTION_PARSER,
    REP_GROUP_MEMBERS_TABLE,
    SERVER_VARIABLES,
    SERVER_VERSION,
    check_server_requirements)
from mysql_gadgets.common.user import (change_user_privileges, User)
from mysql_gadgets.common.req_checker import (DEFAULT, ONE_OF, NOT_IN)
from mysql_gadgets.exceptions import GadgetError


class TestGroupReplication(GadgetsTestCase):
    """Test class for mysql_gadgets.common.group_replication
    """

    @property
    def num_servers_required(self):
        """Property defining the number of servers required by the test.
        """
        return 1

    def setUp(self):
        """ Setup server connection
        """
        server_cnx = {'conn_info': self.options[SERVER_CNX_OPT][0]}
        self.server1 = Server(server_cnx)
        self.server1.connect()
        # default user
        self.server1.exec_query("drop user if exists 'rpl_user'")
        qry_key1 = ("select MEMBER_HOST, MEMBER_PORT from {0}"
                    "".format(REP_GROUP_MEMBERS_TABLE))
        qry_key2 = "show variables like 'group_replication_%'"
        frozen_queries = {qry_key1: [[self.server1.host, self.server1.port]],
                          qry_key2: [("group_replication_group_name", "name"),
                                     ("group_replication_start_on_boot", "ON"),
                                     ("group_replication_group_seeds", "")]}
        variables = {GR_LOCAL_ADDRESS: "localhost:3307"}
        self.server = get_mock_server(self.server1, queries=frozen_queries,
                                      variables=variables)
        # Set directory with option files for tests.
        self.option_file_dir = os.path.normpath(
            os.path.join(__file__, "..", "std_data", "option_files"))

        self.session_track_system_variables_bkp = self.server1.exec_query(
            "select @@global.session_track_system_variables")[0][0]

    def tearDown(self):
        """ Disconnect base server (for all tests).
        """
        queries = [
            "drop user if exists 'rpl_user'",
            "drop user if exists 'replic_user'@'localhost'",
            "drop user if exists 'create_rpl'@'127.0.0.1'",
            "drop user if exists 'replic_user'",
            "set @@global.session_track_system_variables='{0}'".format(
                self.session_track_system_variables_bkp),
        ]
        for query in queries:
            self.server1.exec_query(query)
        self.server1.disconnect()

    def test_get_gr_members(self):
        """Tests get_gr_members method"""

        self.assertIsInstance(get_gr_members(self.server)[0],
                              Server, "An object of class server was "
                              "expected")

    def test__print_check_unique_id_results(self):
        """Tests _print_check_unique_id_results method"""
        server_id_res = {"pass": True}
        _print_check_unique_id_results(server_id_res)

        server_id_res = {"pass": False, "duplicate": None}
        _print_check_unique_id_results(server_id_res)

        server_id_res = {"pass": False, "duplicate": self.server}
        _print_check_unique_id_results(server_id_res)

    def test_print_check_server_version_results(self):
        """Tests _print_check_server_version_results method"""
        server_ver_res = {"pass": False, SERVER_VERSION: [5, 6, 4]}
        self.assertIsNone(_print_check_server_version_results(server_ver_res))

    def test_get_gr_name_from_peer(self):
        """Tests the get_gr_name_from_peer method"""

        self.assertEqual(get_gr_name_from_peer(self.server),
                         "'b7286041-3016-11e6-ba52-507b9d87510a'")

    def test_get_gr_local_address_from(self):
        """Tests the get_gr_local_address_from method"""

        self.assertEqual(get_gr_local_address_from(self.server),
                         "'localhost:3307'")

    def test_get_gr_configs_from_instance(self):
        """Tests the get_gr_configs_from_instance function"""
        res = get_gr_configs_from_instance(self.server)
        # check that result is as expected from the mock server
        self.assertEqual(
            res, OrderedDict([('group_replication_group_name', 'name'),
                              ('group_replication_start_on_boot', 'ON'),
                              ('group_replication_group_seeds', '')]))

    def test__print_check_variables_results(self):
        """Tests _print_check_variables_results method"""
        _format_table_results({"pass": True})
        server_var_res = {
            "pass": False,
            "var_name": (True, "expected val", "current val"),
            "var_name2": (False, "exp_val", "cur_val"),
        }
        self.assertTrue(len(_format_table_results(server_var_res)), 2)

    def test_check_server_requirements(self):
        """Tests check_server_requirements method"""
        skip_if_not_GR_approved(self.server1)
        self.server.exec_query("drop user if exists 'new_rpl_user'")
        self.server.exec_query("drop user if exists 'replic_user'@'%'")
        self.server.exec_query("drop user if exists 'replic_user'@'localhost'")
        # Test with default values, server.user must have SUPER
        options = {
            "replication_user": "replic_user@%",
            "rep_user_passwd": "rplr_pass",
        }
        rpl_settings = get_rpl_usr(options)
        req_dict = get_req_dict(self.server, rpl_settings["replication_user"])
        self.assertTrue(check_server_requirements(self.server, req_dict,
                                                  rpl_settings, verbose=True,
                                                  dry_run=False,
                                                  skip_backup=True))

        self.assertTrue(User(self.server, "replic_user@%", None).exists(),
                        "User was not created on check_server_requirements")

        # Test using an admin user without CREATE USER privilege.
        grant_list = ["SELECT"]
        self.server.exec_query("DROP USER IF EXISTS "
                               "'replic_user'@'localhost'")
        change_user_privileges(self.server, "replic_user", 'localhost',
                               "rplr_pass", grant_list=grant_list,
                               create_user=True)

        server2 = Server({"conn_info": "replic_user@localhost:{0}"
                                       "".format(self.server.port)})
        server2.passwd = "rplr_pass"
        server2.connect()
        qry_key = ("select MEMBER_HOST, MEMBER_PORT from {0}"
                   "".format(REP_GROUP_MEMBERS_TABLE))

        frozen_queries = {qry_key: [[server2.host, server2.port]]}
        mock_server = get_mock_server(server2, variables=frozen_queries)

        options = {
            "replication_user": "new_rpl_user",
            "rep_user_passwd": "rpl_pass",
        }
        rpl_settings = get_rpl_usr(options)
        req_dict = get_req_dict(server2, rpl_settings["replication_user"])

        # expect GadgetError: Query failed: No required privileges
        #                                   to create the replication user
        with self.assertRaises(GadgetError) as test_raises:
            check_server_requirements(mock_server, req_dict, rpl_settings,
                                      verbose=True, dry_run=False,
                                      skip_backup=True)
        exception = test_raises.exception
        self.assertTrue("required privileges to create" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # Test existing user and admin user without REPLICATION SLAVE grant
        grant_list = ["SELECT"]
        revoke_list = ["REPLICATION SLAVE"]
        change_user_privileges(self.server, "new_rpl_user", "%",
                               grant_list=grant_list, revoke_list=revoke_list,
                               create_user=True, with_grant=True)
        revoke_list = ["REPLICATION SLAVE"]
        change_user_privileges(self.server, "replic_user", "%",
                               revoke_list=revoke_list)

        # expect GadgetError: does not have the REPLICATION SLAVE privilege,
        #                     and can not be granted by.
        with self.assertRaises(GadgetError) as test_raises:
            check_server_requirements(mock_server, req_dict, rpl_settings,
                                      verbose=True, dry_run=False,
                                      skip_backup=True)
        exception = test_raises.exception
        self.assertTrue("does not have the REPLICATION SLAVE privilege, "
                        "and can not be granted by" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        self.assertTrue("SUPER privilege required to disable the binlog."
                        in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # self.server.exec_query("drop user if exists 'replic_user'@'%'")
        # Test existing user and admin user without REPLICATION SLAVE grant
        grant_list = ["SELECT"]
        revoke_list = ["REPLICATION SLAVE"]
        change_user_privileges(self.server, "new_rpl_user", "%",
                               grant_list=grant_list, revoke_list=revoke_list)
        grant_list = ["REPLICATION SLAVE", "SUPER"]
        change_user_privileges(self.server, "replic_user", "localhost",
                               grant_list=grant_list, with_grant=True)

        # reset session to get new privileges.
        server2.disconnect()
        server2.connect()
        mock_server = get_mock_server(server2, variables=frozen_queries)

        self.assertTrue(check_server_requirements(mock_server, req_dict,
                                                  rpl_settings, verbose=True,
                                                  dry_run=False,
                                                  skip_backup=True))

        # Test existing rpl user and admin user without grant
        # admin user: replic_user
        # rpl user: new_rpl_user
        grant_list = ["REPLICATION SLAVE"]
        change_user_privileges(self.server, "new_rpl_user", "%",
                               grant_list=grant_list)
        grant_list = ["SELECT", "CREATE USER"]
        change_user_privileges(self.server, "create_rpl", "127.0.0.1",
                               user_passwd="c_pass", grant_list=grant_list,
                               create_user=True, with_grant=True)
        server3 = Server({"conn_info": "create_rpl@127.0.0.1:{0}"
                                       "".format(self.server.port)})
        server3.passwd = "c_pass"
        server3.connect()
        req_dict3 = get_req_dict(server3, rpl_settings["replication_user"])
        # expect GadgetError: No required privileges
        #                     to grant Replication Slave privilege
        with self.assertRaises(GadgetError) as test_raises:
            check_server_requirements(server3, req_dict3, rpl_settings,
                                      verbose=True, dry_run=False,
                                      skip_backup=True)
        exception = test_raises.exception
        self.assertTrue("SUPER privilege needed to run the CHANGE MASTER "
                        "command" in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # Test invalid server_id
        mock_server = get_mock_server(self.server,
                                      variables={"server_id": "0"})
        req_dict = get_req_dict(self.server, rpl_settings["replication_user"])

        # expect GadgetError: server_id not valid
        with self.assertRaises(GadgetError) as test_raises:
            check_server_requirements(mock_server, req_dict, rpl_settings,
                                      verbose=True, dry_run=False,
                                      skip_backup=True)
        exception = test_raises.exception
        self.assertTrue("is not valid, it must be a positive integer"
                        in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # Test duplicate server_id
        mock_server = get_mock_server(self.server,
                                      variables={"server_id": "101"})
        req_dict["SERVER_ID"] = {"peers": [mock_server]}
        # expect GadgetError: server_id is already used by peer
        with self.assertRaises(GadgetError) as test_raises:
            check_server_requirements(mock_server, req_dict, rpl_settings,
                                      verbose=True, dry_run=False,
                                      skip_backup=True)
        exception = test_raises.exception
        self.assertTrue("is already used by peer"
                        in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        # Test existing user and admin with required grants
        req_dict = get_req_dict(self.server, rpl_settings["replication_user"])
        grant_list = ["REPLICATION SLAVE"]
        change_user_privileges(self.server, "new_rpl_user", "%",
                               grant_list=grant_list)
        self.assertTrue(check_server_requirements(self.server, req_dict,
                                                  rpl_settings, verbose=True,
                                                  dry_run=False,
                                                  skip_backup=True))

        # Tests server variables not meet required values
        req_dict = get_req_dict(self.server, rpl_settings["replication_user"])
        req_dict[SERVER_VARIABLES] = {
            "log_bin": {ONE_OF: ("0",)},
            "binlog_format": {ONE_OF: ("",)},
            "binlog_checksum": {ONE_OF: ("",)},

            "gtid_mode": {ONE_OF: ("OFF",)},
            "log_slave_updates": {ONE_OF: ("0",)},
            "enforce_gtid_consistency": {ONE_OF: ("OFF",)},
        }
        # expect GadgetError: change the configuration values
        with self.assertRaises(GadgetError) as test_raises:
            check_server_requirements(self.server, req_dict, rpl_settings,
                                      verbose=True, dry_run=True,
                                      update=False, skip_backup=True)
        exception = test_raises.exception
        self.assertIn("on server {0} are incompatible with Group "
                      "Replication.".format(self.server),
                      exception.errmsg,
                      "The exception message was not the expected. {0}"
                      "".format(exception.errmsg))

        # Test server version
        req_dict[SERVER_VERSION] = "99.9.9"
        # expect GadgetError: Query failed: server version
        with self.assertRaises(GadgetError) as test_raises:
            check_server_requirements(self.server, req_dict, rpl_settings,
                                      verbose=True, dry_run=True,
                                      update=False, skip_backup=True)
        exception = test_raises.exception
        self.assertTrue("does not meet the required MySQL server version"
                        in exception.errmsg,
                        "The exception message was not the expected. {0}"
                        "".format(exception.errmsg))

        self.server.exec_query("drop user if exists 'replic_user'")
        self.server.exec_query("drop user if exists 'create_rpl'")
        self.server.exec_query("drop user if exists 'new_rpl_user'")

    def test_check_server_requirements_config_vars(self):
        """Tests check_server_requirements method"""

        # The dict object with the requirements to check.
        req_dict = {}
        # Add the variables to check on server.
        req_dict[SERVER_VARIABLES] = {
            'session_track_system_variables': {ONE_OF: ("",)},
            "log_bin": {ONE_OF: ("1", "ON")},
        }
        self.server1.exec_query(
            "set @@global.session_track_system_variables='time_zone'")

        # Allow the variable to change dynamically.
        dynamic_vars = set(DYNAMIC_SERVER_VARS)
        dynamic_vars.add('session_track_system_variables')

        self.assertTrue(
            check_server_requirements(self.server1, req_dict, None,
                                      dry_run=False, skip_backup=True,
                                      update=True, dynamic_vars=dynamic_vars))

        self.assertEqual(self.server1.select_variable(
            'session_track_system_variables', "global"), "")

        # Change the variables value.
        req_dict[SERVER_VARIABLES]['session_track_system_variables'] = {
            ONE_OF: ("",)}

        # check_server_requirements updates by default
        self.assertTrue(
            check_server_requirements(self.server1, req_dict, None,
                                      verbose=True, dry_run=False,
                                      skip_backup=True,
                                      dynamic_vars=dynamic_vars))

        self.assertEqual(self.server1.select_variable(
            'session_track_system_variables', "global"), "")

        work_file = os.path.join(self.option_file_dir, "my_test.cnf")
        orig_file = os.path.join(self.option_file_dir, "my.cnf")

        # test requirements from a configuration file.
        shutil.copy(orig_file, work_file)

        req_dict[OPTION_PARSER] = MySQLOptionsParser(work_file)
        req_dict[CONFIG_SETTINGS] = {
            'session_track_system_variables': {ONE_OF: ("autocommit",)},
            "log_bin": {NOT_IN: ("ON", "1", "<not set>"),
                        DEFAULT: "0"},
        }

        try:
            opt_file_parser = MySQLOptionsParser(work_file)
            # make sure section mysqld exist.
            self.assertTrue(opt_file_parser.has_section('mysqld'))

            if opt_file_parser.has_option('mysqld', 'log_bin'):
                # delete it.
                opt_file_parser.remove_option('mysqld', 'log_bin')

            self.assertFalse(opt_file_parser.has_option('mysqld', 'log_bin'))

            # write the changes
            opt_file_parser.write()

            self.assertTrue(
                check_server_requirements(self.server1, req_dict, None,
                                          verbose=True, dry_run=False,
                                          skip_backup=True,
                                          dynamic_vars=dynamic_vars))

            # check_server_requirements should change the value of
            # session_track_system_variables and log_bin
            opt_file_parser = MySQLOptionsParser(work_file)
            self.assertTrue(opt_file_parser.has_option('mysqld', 'log_bin'))
            self.assertEqual(opt_file_parser.get('mysqld', 'log_bin'), "0")
            self.assertTrue(opt_file_parser.has_option(
                'mysqld', 'session_track_system_variables'))
            self.assertEqual(opt_file_parser.get(
                'mysqld', 'session_track_system_variables'), "autocommit")

            req_dict[OPTION_PARSER] = MySQLOptionsParser(work_file)
            req_dict[CONFIG_SETTINGS] = {
                'session_track_system_variables': {ONE_OF: ("<no value>",)},
                "log_bin": {NOT_IN: ("OFF", "0", "<not set>"),
                            DEFAULT: "<no value>"},
            }

            self.assertTrue(
                check_server_requirements(self.server1, req_dict, None,
                                          verbose=True, dry_run=False,
                                          skip_backup=True,
                                          dynamic_vars=dynamic_vars))

            # check_server_requirements should change the value of
            # session_track_system_variables and log_bin
            opt_file_parser = MySQLOptionsParser(work_file)
            self.assertTrue(opt_file_parser.has_option('mysqld', 'log_bin'))
            self.assertEqual(opt_file_parser.get('mysqld', 'log_bin'), None)
            self.assertTrue(opt_file_parser.has_option(
                'mysqld', 'session_track_system_variables'))
            self.assertEqual(opt_file_parser.get(
                'mysqld', 'session_track_system_variables'), None)

        finally:
            try:
                os.remove(work_file)
            except OSError:
                pass
