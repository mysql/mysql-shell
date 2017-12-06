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
import logging

from unit_tests.mock_server import get_mock_server
from unit_tests.utils import (GadgetsTestCase, SERVER_CNX_OPT)

from mysql_gadgets.common.server import Server
from mysql_gadgets.exceptions import GadgetError
from mysql_gadgets.common.req_checker import (ONE_OF,
                                              RequirementChecker,
                                              SERVER_VARIABLES,
                                              SERVER_VERSION,
                                              USER_PRIVILEGES,)


class Test(GadgetsTestCase):
    """Unit Test Class for the mysql_gadgets.common.req_checker module.
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
        """ restore server
        """
        self.server.disconnect()

    def test_requirement_checker_use_defaults(self):
        """ Test requirement_checker fail with positive values.
        """
        req_check = RequirementChecker()
        results = req_check.check_requirements()

        logging.debug("check_requirements result %s", results)
        self.assertTrue(results['pass'], "Check was expected to Pass.")

    def test_requirement_checker_no_reqs(self):
        """ Test test_requirement_checker_no_reqs pass without requirements.
        """
        req_dict = {}

        req_check = RequirementChecker(req_dict, server=self.server)
        results = req_check.check_requirements()

        logging.debug("check_requirements result %s", results)
        self.assertTrue(results['pass'], "Check was expected to Pass.")

    def test_requirement_checker_server_var_empty(self):
        """ Test requirement_checker pass with no variables to test.
        """
        req_dict = {
            SERVER_VARIABLES: {}
        }

        req_check = RequirementChecker(req_dict, server=self.server)
        results = req_check.check_requirements()

        logging.debug("check_requirements result %s", results)
        self.assertTrue(results['pass'], "Check was expected to Pass.")

    def test_requirement_checker_server_invalid_reqs(self):
        """ Test requirement_checker pass with invalid tests.
        """
        req_dict = {
            "invalid": {}
        }

        req_check = RequirementChecker(req_dict, server=self.server)
        results = req_check.check_requirements()

        logging.debug("check_requirements result %s", results)
        self.assertTrue(results['pass'], "Check was expected to Pass.")

    def test_requirement_checker_allvalues_are_checked(self):
        """ Test requirement_checker all values tested regardless of result.
        """
        res = self.server.exec_query("show databases")
        logging.debug(res)
        req_dict = {
            SERVER_VARIABLES: {
                "log_bin": {ONE_OF: ("1",)},
                "binlog_format": {ONE_OF: ("ROW",)},
                "binlog_checksum": {ONE_OF: ("NONE",)},

                "gtid_mode": {ONE_OF: ("1", "ON")},
                "log_slave_updates": {ONE_OF: ("1",)},
                "enforce_gtid_consistency": {ONE_OF: ("1", "ON")},

                "master_info_repository": {ONE_OF: ("TABLE",)},
                "relay_log_info_repository": {ONE_OF: ("TABLE",)},
            },
            SERVER_VERSION: "5.7.10"
        }
        test_list = req_dict[SERVER_VARIABLES].keys()

        req_check = RequirementChecker(req_dict, server=self.server)
        results = req_check.check_requirements()
        res_list = results[SERVER_VARIABLES]
        res_list.pop('pass')
        for var in res_list.keys():
            self.assertIn(var, test_list, "{0} was not found".format(var))

        for var in test_list:
            self.assertIn(var, res_list, "{0} was not found".format(var))

    def test_requirement_checker_fail(self):
        """ Test requirement_checker fail check.
        """
        logging.debug("\n-- test_requirement_checker_fail")
        res = self.server.exec_query("SET SQL_LOG_BIN=1")
        logging.debug(res)
        req_dict = {
            SERVER_VARIABLES: {
                "sql_log_bin": {ONE_OF: ("0",)},
            },
            SERVER_VERSION: [9, 9, 9]
        }

        req_check = RequirementChecker(req_dict, server=self.server)
        results = req_check.check_requirements()

        logging.debug("check_requirements result %s", results)
        self.assertTupleEqual(results[SERVER_VARIABLES]["sql_log_bin"],
                              (False, '0', '1'),
                              "sql_log_bin value is not correct in result.")
        self.assertFalse(results['pass'], "Check was expected to fail.")

    def test_requirement_checker_logging(self):
        """ Test requirement_checker logging.
        """
        req_dict = {
            SERVER_VARIABLES: {
                "sql_log_bin": {ONE_OF: ("1",)},
            },
            SERVER_VERSION: "5.7.11"
        }

        req_check = RequirementChecker(req_dict, self.server)
        results = req_check.check_requirements()

        self.assertNotEqual(results['pass'], None,
                            "Check must return a value.")

    def test_requirement_checker_server_version(self):
        """ Test requirement_checker server version.
        """
        req_dict = {
            SERVER_VARIABLES: {
                "sql_log_bin": {ONE_OF: ("1",)},
            },
            SERVER_VERSION: "5.7.10"
        }

        req_check = RequirementChecker(req_dict, server=self.server)
        results = req_check.check_requirements()

        logging.debug("check_requirements result %s", results)
        self.assertTrue(results['pass'], "Check was expected to pass.")
        self.assertTupleEqual(results[SERVER_VARIABLES]["sql_log_bin"],
                              (True, '1', '1'),
                              "sql_log_bin value is not correct in result.")

    def test_requirement_checker_server_version_fail(self):
        """ Test requirement_checker fail due to server version.
        """
        req_dict = {
            SERVER_VARIABLES: {
                "sql_log_bin": {ONE_OF: ("1",)},
            },
            SERVER_VERSION: "11.1.1"
        }

        req_check = RequirementChecker(req_dict, server=self.server)
        results = req_check.check_requirements()
        logging.debug("check_requirements result %s", results)
        self.assertFalse(results['pass'], "Check was expected to fail.")

    def test_requirement_checker_alt_server(self):
        """ Test requirement_checker fail with positive values.
        """

        res = self.server.exec_query("SET GLOBAL show_compatibility_56=1")
        logging.debug(res)
        req_dict = {
            SERVER_VARIABLES: {
                "show_compatibility_56": {ONE_OF: ("0",)},
            }
        }

        req_check = RequirementChecker(req_dict)
        results = req_check.check_requirements(self.server)

        logging.warning("check_requirements result %s", results)

        self.assertFalse(results['pass'], "Check was expected to fail.")
        self.assertTupleEqual(
            results[SERVER_VARIABLES]["show_compatibility_56"],
            (False, '0', '1'),
            "show_compatibility_56 is not correct in result."
        )
        self.server.exec_query("SET GLOBAL show_compatibility_56=0")

    def test_requirement_checker_invalid_version_format(self):
        """ Test requirement_checker fail due to values found.
        """
        req_dict = {
            SERVER_VERSION: "x.y.z"
        }

        req_check = RequirementChecker(req_dict, self.server)

        # expect GadgetError: Query failed: Unknown system variable
        with self.assertRaises(GadgetError) as test_raises:
            results = req_check.check_requirements()
            logging.debug("check_requirements result %s", results)
        exception = test_raises.exception
        logging.debug(dir(exception))
        self.assertTrue("does not have a valid format" in exception.errmsg,
                        "The exception message was not the expected")

    def test_requirement_checker_unknown_system_variable(self):
        """ Test requirement_checker fail due to Unknown system variable.
        """
        req_dict = {
            SERVER_VARIABLES: {
                "invalid_var": {ONE_OF: ("no_exist")},
            }
        }

        req_check = RequirementChecker(req_dict, server=self.server)

        results = req_check.check_requirements()

        self.assertFalse(results["pass"])

    def test_requirement_checker_no_server(self):
        """ Test requirement_checker no server has been set to check.
        """
        req_dict = {
            SERVER_VARIABLES: {
                "invalid_var": {ONE_OF: ("no_exist",)},
            }
        }

        req_check = RequirementChecker(req_dict)

        # expect GadgetDBError: Query failed: Unknown system variable
        with self.assertRaises(GadgetError) as test_raises:
            results = req_check.check_requirements()
            logging.debug("check_requirements result %s", results)
        exception = test_raises.exception
        logging.debug(dir(exception))
        self.assertTrue("no server has been set to check" in exception.errmsg,
                        "The exception message was not the expected")

    def test_requirement_checker_user_privileges(self):
        """Test requirement_checker user privileges
        """
        self.server.exec_query("drop user IF EXISTS 'john_doe'@'localhost'")

        req_dict = {
            SERVER_VARIABLES: {
                "sql_log_bin": {ONE_OF: ("1",)},
            },
            SERVER_VERSION: "5.7.10",
            USER_PRIVILEGES: {
                "john_doe@localhost": {"SUPER", "REPLICATION SLAVE", "INSERT",
                                       "UPDATE", "DELETE"}
            }

        }

        req_check = RequirementChecker(req_dict, server=self.server)
        results = req_check.check_requirements()

        logging.debug("check_requirements result %s", results)
        self.assertFalse(results['pass'], "Check was expected to fail.")
        self.assertEqual(results[USER_PRIVILEGES]["john_doe@localhost"],
                         ['NO EXISTS!'],
                         "missing privileges value is not correct in result.")

        self.server.exec_query("create user 'john_doe'@'localhost'")
        self.server.exec_query("grant SELECT,INSERT,UPDATE on *.* to "
                               "'john_doe'@'localhost'")

        self.server.exec_query("grant REPLICATION SLAVE on *.* to"
                               " 'john_doe'@'localhost'")

        results = req_check.check_requirements()
        logging.debug("check_requirements result %s", results)
        self.assertEqual(results[USER_PRIVILEGES]["john_doe@localhost"],
                         "DELETE and SUPER",
                         "sql_log_bin value is not correct in result.")
        self.assertFalse(results['pass'], "Check was expected to fail.")

        self.server.exec_query("drop user IF EXISTS 'john_doe'@'localhost'")

    def test_check_unique_id(self):
        """Tests check_unique_id method"""
        # Test duplicated server_id
        mock_server = get_mock_server(self.server)
        server_values = {"peers": [mock_server]}
        req_check = RequirementChecker()
        results = req_check.check_unique_id(server_values, mock_server)
        self.assertFalse(results["pass"])
        self.assertEqual(results["duplicate"], mock_server)

        # Test invalid server_id = 0
        frozen_variables = {"server_id": "0"}
        mock_server_idz = get_mock_server(self.server,
                                          variables=frozen_variables)
        results = req_check.check_unique_id(server_values, mock_server_idz)
        self.assertFalse(results["pass"])

        # Servers with different server_id
        frozen_variables = {"server_id": "777"}
        mock_server2 = get_mock_server(self.server,
                                       variables=frozen_variables)
        server_values = {"peers": [mock_server, mock_server2]}
        results = req_check.check_unique_id(server_values, self.server)
        self.assertTrue(results["pass"])

    def test_check_user_privileges(self):
        """Tests check_user_privileges method.
        """
        req_check = RequirementChecker()
        self.server.exec_query("Drop USER if exists 'check_user_privs'@'%'")
        self.server.exec_query("CREATE USER 'check_user_privs'@'%'")
        user_str = "check_user_privs@%"
        priv_values = {user_str: ["NO EXISTING GRANT"]}
        results = req_check.check_user_privileges(priv_values, self.server)
        self.assertFalse(results["pass"])

        self.server.exec_query("Drop USER if exists 'check_user_privs'@'%'")
