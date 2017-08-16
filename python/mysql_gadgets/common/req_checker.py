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
This module provides methods (through the RequirementChecker class) to check
requirements (option values, version, user privileges, etc.) for a MySQL
server.
"""
import logging

from mysql_gadgets.exceptions import (GadgetError, GadgetQueryError)
from mysql_gadgets.common.config_parser import MySQLOptionsParser
from mysql_gadgets.common.user import User

# Get common logger
_LOGGER = logging.getLogger(__name__)

CONFIG_SETTINGS = "CONFIG_SETTINGS"
OPTION_PARSER = "OPTION_PARSER"
SERVER_ID = "SERVER_ID"
SERVER_VARIABLES = "SERVER_VARIABLES"
SERVER_VERSION = "SERVER_VERSION"
USER_PRIVILEGES = "USER_PRIVILEGES"
PEER_SERVER_VARIABLES = "PEER_SERVER_VARIABLES"
MTS_SETTINGS = "MTS_SETTINGS"

# Comparison key names:
DEFAULT = "DEFAULT"
NOT_IN = "NOT IN"
ONE_OF = "ONE OF"
ALL_OF = "ALL OF"

GR_COMPLIANCE_SKIP_SCHEMAS = ("('mysql', 'sys', 'performance_schema', "
                              "'information_schema')")
GR_COMPLIANCE_SKIP_ENGINES = "('InnoDB', 'MEMORY')"


def check_option(var_name, value, cur_val, results):
    """Check the option value against the current value.

    :param var_name: Option name
    :type var_name: string
    :param value: A dictionary with values to check
    :type value: dict
    :param cur_val: The current value of the option
    :type cur_val: string
    :param results: A dictionary to save the results
    :type results: dict

    :return: A tuple of (test result, dict with those options with value set
             to "<no value>")
    :rtype: (bool, dict)
    """
    missing = {}
    valid = True

    # Test "not in" comparison, if value not set skip.
    if NOT_IN in value.keys():
        # undesired_values are strict unwanted values.
        undesired_values = [val.upper() for val in value[NOT_IN]]
        if cur_val == "<no value>" and cur_val != value[DEFAULT]:
            # Log test result
            _LOGGER.debug("Fail: option %s is not set.", var_name)
            # Set result for this option
            # Log test result
            _LOGGER.debug("Adding option %s to missing list with "
                          "value %s.", var_name, value[DEFAULT])
            res = (False, value[DEFAULT], cur_val)
            missing[var_name] = value[DEFAULT]
            # check validation set to fail.
            valid = False
        # Test pass if current value not found not is any of
        # the given undesired_values.
        elif cur_val.upper() not in undesired_values:
            # Log test result
            _LOGGER.debug('OK: value %s is not in %s', cur_val,
                          value[NOT_IN])
            # set cur_val as value to show as is correct
            res = (True, cur_val, cur_val)
        else:
            # Log test result
            _LOGGER.debug("Fail: value %s can't be in %s",
                          cur_val, value[NOT_IN])
            # Set result for this option
            if DEFAULT in value.keys():
                res = (False, value[DEFAULT], cur_val)
            else:
                res = (False, "", cur_val)
            # check validation set to fail.
            valid = False
        results[var_name] = res

    # The option is not set, save to missing
    elif NOT_IN in value.keys() and cur_val is None:
        # the option needs to be set only if default is provided
        if DEFAULT in value.keys():
            missing[var_name] = value[DEFAULT]
            valid = False

    # Test "one of" comparison.
    elif ONE_OF in value.keys():
        if cur_val is not None:
            # Test pass if current value is one of the required
            # undesired_values.
            undesired_values = [val.upper() for val in value[ONE_OF]]
            if cur_val.upper() in undesired_values:
                # Log test result
                _LOGGER.debug('OK: value %s is one of %s',
                              cur_val, value[ONE_OF])
                # set cur_val as value to show as is correct
                res = (True, cur_val, cur_val)
            else:
                # Log test result
                _LOGGER.debug('Fail: value %s is not one of %s',
                              cur_val, value[ONE_OF])
                # Set result for this option
                res = (False, value[ONE_OF][0], cur_val)
                # check validation set to fail.
                valid = False
            results[var_name] = res
        # The value is missing, set it to the first.
        else:
            # the option needs to be set:
            missing[var_name] = value[ONE_OF][0]
            valid = False

    # Test "all of" comparison.
    elif ALL_OF in value.keys():
        if cur_val is not None:
            # Test pass if current value contains all the
            # required undesired_values.
            cur_vals = cur_val.upper().replace(' ', '').split(',')
            # missing values, values not found in current values
            undesired_values = [val.upper() for val in value[ALL_OF]]
            miss_vals = [val for val in undesired_values if val
                         not in cur_vals]
            if not miss_vals:
                # Log test result
                _LOGGER.debug('OK: current value %s has all '
                              'required %s', cur_val,
                              value[ALL_OF])
                # set cur_val as value to show  as is correct
                res = (True, cur_val, cur_val)
            else:
                # Log test result
                _LOGGER.debug('Fail: values %s not contains %s',
                              cur_val, miss_vals)
                # Set result for this option
                res = (False, ",".join(value[ALL_OF]), cur_val)
                # check validation set to fail.
                valid = False
            results[var_name] = res
        # The value is missing, set it to the first.
        else:
            # the option needs to be set
            missing[var_name] = ",".join(value[ALL_OF])
            valid = False

    return valid, missing


class RequirementChecker(object):
    """This class is used to check the requirements on the given server.

    This class performs the following tasks:
        - Verifies the server variables for a required value.
        - Verifies the server version.
        - Verifies the user accounts privileges.

    """

    def __init__(self, req_dict=None, server=None):
        """Sets the initial requirements and the server to verify.

        :param req_dict: a dictionary with the requirements to check in the
                         form:
            SERVER_VARIABLES: {"variable name": <required value>},
                              [{"variable name": <required value>}]
            SERVER_VERSION: "X.Y.Z"
            USER_PRIVILEGES: "<Usr _name>@<host>": <List of privileges>

        :type req_dict:  dict
        :param server:   The server to check
        :type server:    Server instance
        """
        if req_dict is None:
            req_dict = {}

        # Set server attribute if given
        self.server = None
        if server is not None:
            _LOGGER.debug("The server: %s has been set to check", server)
            self.server = server

        # Set requirements dictionary if given
        self.req_dict = req_dict

    def _get_server(self, alt_server=None, raise_error=True):
        """Chooses the server that should be use.

        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :raise GadgetError: Raises exception if no server has been set to
                            check.

        :return the server that should be use
        :rtype: Server instance
        """
        # Validate alternative server as if given as first option
        server = None
        if alt_server is not None:
            server = alt_server
        # Validate attribute server if set as second option
        if server is None:
            server = self.server
        # If not server to check raise use exception
        if server is None and raise_error:
            msg = "no server has been set to check."
            _LOGGER.error(msg)
            raise GadgetError(msg)

        return server

    def check_requirements(self, alt_server=None):
        """Verifies if the given server fulfill all the requirements

        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :return: a dictionary with the results, including the key "pass" with
                 True if all the requirement tests passed.
        :rtype:  dict
        """

        # Store Results
        results = {}

        # Validate each requirement
        for req_name, req_val in self.req_dict.items():
            if req_name in SERVER_VARIABLES:
                _LOGGER.info("* Checking server options")
                results[req_name] = self.check_variable_values(req_val,
                                                               alt_server)
            if req_name in SERVER_VERSION:
                _LOGGER.info("* Checking server version")
                results[req_name] = self.check_server_version(req_val,
                                                              alt_server)

            if req_name in USER_PRIVILEGES:
                _LOGGER.info("* Checking user privileges")
                results[req_name] = self.check_user_privileges(req_val,
                                                               alt_server)

        valid = True
        for req_name, result in results.items():
            if not result["pass"]:
                valid = False
                break

        results["pass"] = valid

        # Return result
        return results

    def check_variable_values(self, var_values, alt_server=None):
        """Checks the server variable values.

        :param var_values:  dictionary with the variable name as keys and the
                            required value as his value.
        :type var_values:   dict
        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :return: a dictionary with the results, including the key "pass" with
                 True if all the variables match the required value.
        :rtype:  dict
        """
        # Dictionary should not be used as argument for logger, since it
        # duplicates backslash. Convert to string and convert \\ back to \.
        dic_msg = str(var_values)
        dic_msg = dic_msg.replace("\\\\", "\\")
        _LOGGER.debug('Option checking started: %s', dic_msg)
        # Get the correct server to Validate
        server = self._get_server(alt_server=alt_server)

        # Store Result
        results = {}
        valid = True

        for var_name, value in var_values.items():
            # Value of type Dictionary can hold more complex requirements
            # (namely unwanted values or one of...):
            # key names for comparison:
            #    "NOT IN": The current value must not match any on values
            #    "ONE OF": The current value must match one of
            #    "All OF": The current value must have all the values from the
            #              values list.
            # The value of each key it must be a list of values to use during
            # the comparison.
            if isinstance(value, dict):
                _LOGGER.debug("Checking option: '%s' ", var_name)
                try:
                    cur_val = server.select_variable(var_name)
                    _LOGGER.debug("Option current value: '%s' ", cur_val)
                except GadgetQueryError:
                    cur_val = "<not set>"
                    _LOGGER.debug("Option '%s' does not exists on server %s",
                                  var_name, server)

                if cur_val == "" or cur_val is None:
                    _LOGGER.debug('Option found but with empty value')
                    cur_val = "<no value>"

                res = check_option(var_name, value, cur_val, results)
                valid = res[0] and valid

            else:
                raise GadgetError("The requirements format of option {0} "
                                  "are not valid.".format(var_name))

        _LOGGER.debug('Options check result: %s', valid)
        results["pass"] = valid
        return results

    def check_config_settings(self, var_values, location, alt_server=None):
        """Checks the server variable undesired_values.

        :param var_values:  dictionary with the variable name as keys and the
                            required value as his value.
        :type var_values:   dict
        :param location:    Path to the option file or an option parser.
        :type location:     string or MySQLOptionsParser instance.
        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :raise GadgetError: If location is not an instance of string or
                            MySQLOptionsParser.

        :return: a dictionary with the results, including the key "pass" with
                 True if all the variables match the required value.
        :rtype:  dict
        """
        # Dictionary should not be used as argument for logger, since it
        # duplicates backslash. Convert to string and convert \\ back to \.
        dic_msg = str(var_values)
        dic_msg = dic_msg.replace("\\\\", "\\")
        _LOGGER.debug('Server config settings checking: %s', dic_msg)
        # Get the correct server to Validate, for check the file the server
        # is optional.
        server = self._get_server(alt_server=alt_server, raise_error=False)

        # Option parser
        if isinstance(location, MySQLOptionsParser):
            opt_parser = location
        elif isinstance(location, str):
            opt_parser = MySQLOptionsParser(location)
        else:
            raise GadgetError("An instance of string or MySQLOptionsParser "
                              "was expected not %s", location)

        # Store Result
        results = {}
        # Check is valid only if none option is faulty.
        valid = True
        section = "mysqld"

        if server is not None and opt_parser.has_option(section, "port"):
            port_val = opt_parser.get(section, "port")
            if server.port != int(port_val):
                _LOGGER.warning("The port number %s in the option file "
                                "differs from server port number"
                                " %s.", port_val, server.port)
        if server is not None and opt_parser.has_option(section, "host"):
            host_val = opt_parser.get(section, "port")
            if server.host != host_val:
                _LOGGER.warning("The host name %s in the option file "
                                "differs from server host name"
                                " %s.", host_val, server.host)

        missing = {}
        for opt_name, value in var_values.items():
            # Use "_" as standard as is used on the server variables
            opt_name = opt_name.replace("-", "_")

            # Value of type Dictionary can hold more complex requirements
            # (namely unwanted values or one of...):
            # key names for comparison:
            #    "NOT IN": The current value must not match any on values
            #    "ONE OF": The current value must match one of
            #    "All OF": The current value must have all the values from the
            #              values list.
            # The value of each key it must be a list of values to use during
            # the comparison.
            if isinstance(value, dict):
                _LOGGER.debug("Checking option: '%s' ", opt_name)
                if opt_parser.has_option(section, opt_name):
                    # the option is already set:
                    cur_val = opt_parser.get(section, opt_name)
                    _LOGGER.debug("Option current value: '%s' ", cur_val)
                    if cur_val == "" or cur_val is None:
                        _LOGGER.debug('Option found but with empty value')
                        cur_val = "<no value>"
                else:
                    cur_val = "<not set>"
                    _LOGGER.debug("Option does not exists on section: '%s' ",
                                  section)

                res = check_option(opt_name, value, cur_val, results)
                valid = res[0] and valid
                missing = res[1]

            else:
                raise GadgetError("The requirements format of option {0} is "
                                  "not valid.".format(opt_name))

        _LOGGER.debug('Options check result: %s', valid)
        if missing:
            results["missing"] = missing
            results["pass"] = False

        results["pass"] = valid
        return results

    def check_server_version(self, ver_values, alt_server=None):
        """Checks server version

        :param ver_values:  string with the required minimum server version in
                            the form "X.Y.Z".
        :type ver_values:   string
        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :return: a dictionary with the results, including the key "pass" with
                 True if the server is same version or newer otherwise False.
        :rtype:  dict
        """
        _LOGGER.debug('Server version checking: %s', ver_values)
        # Get the correct server to Validate
        server = self._get_server(alt_server=alt_server)

        # Store Result
        results = {}

        # Get the independent values
        try:
            if "." in ver_values:
                ver_vals = ver_values.split(".")
                version = [int(ver_val) for ver_val in ver_vals]
            elif len(ver_values) >= 3:
                version = ver_values[0:3]
            else:
                # The format is not valid.
                raise GadgetError("Unexpected server version format '{0}'"
                                  "".format(ver_values))
        except ValueError:
            msg = ("The given version {0} does not have a valid format 'X.Y.Z'"
                   " or (X, Y, Z)".format(ver_values))
            raise GadgetError(msg)
        except GadgetError:
            raise

        results[SERVER_VERSION] = server.get_version()
        _LOGGER.debug('Server version: %s', results[SERVER_VERSION])

        results["pass"] = results[SERVER_VERSION] >= version
        _LOGGER.debug('Server version check result: %s', results["pass"])
        return results

    def check_user_privileges(self, priv_values, alt_server=None):
        """Verifies the given user's names accounts privileges

        :param priv_values: Dictionary with user name as key and list of
                            privileges as value.
        :type priv_values:  dict
        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :raise GadgetError: If the user does not have SELECT
                            privilege on mysql.user.

        :return: A dictionary with the results, including the key pass with
                 True if the user has the privileges and False otherwise, and
                 a key with the checked users and the missing privileges as
                 value (or 'NO EXISTS!' if user does not exists).
        :rtype:  dict
        """
        _LOGGER.debug('privileges Checking')
        # Get the correct server to Validate
        server = self._get_server(alt_server=alt_server)

        # Store Result
        results = {}
        # Check required privileges
        for user, privs in priv_values.items():
            _LOGGER.debug('User: %s required privileges: %s', user, privs)
            user_obj = User(server, user)
            try:
                # verify the user exists
                if user_obj.exists():
                    missing_privs = user_obj.check_missing_privileges(privs)
                    _LOGGER.debug('missing privileges: %s', missing_privs)
                    results[user] = missing_privs
                    if missing_privs:
                        results["pass"] = False
                else:
                    # The user's result will be set to ['NO EXISTS!']
                    # as [] and None are evaluated to False
                    results[user] = ['NO EXISTS!']
                    results["pass"] = False

            except GadgetQueryError as err:
                if "SELECT command denied" in err.errmsg:
                    raise GadgetError("User {} does not have SELECT privileges"
                                      " on user table; unable to check "
                                      "privileges with this account."
                                      "".format(server.user))
                else:
                    raise

        if "pass" not in results.keys():
            results["pass"] = True
        _LOGGER.debug('Privileges check result: %s', results)
        return results

    def check_unique_id(self, server_values, alt_server=None):
        """Verifies the server id value.

        :param server_values: Dictionary with the server peer list.
                                peers    A list of servers that belong to the
                                         GR group.
        :type server_values:  dict
        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :return: a dictionary with the results, including the key pass with
                 True if the user has the privileges
        :rtype:  dict
        """
        _LOGGER.debug('checking server id uniqueness')
        # Get the correct server to Validate

        server = self._get_server(alt_server=alt_server)
        this_server_id = server.select_variable("server_id")
        _LOGGER.debug('server id = %s', this_server_id)
        server_ids = {}

        # Store Result
        results = {}

        if this_server_id == '0':
            _LOGGER.warning("The server_id can not be 0 ")

            results["pass"] = False
            return results

        # check for duplicates
        peer_list = server_values.get("peers", [])
        for peer_server in peer_list:
            peer_id = peer_server.select_variable("server_id")
            server_ids[peer_id] = peer_server
            _LOGGER.debug("Verifying the peer %s ...", peer_server)
            if peer_id == this_server_id:
                _LOGGER.warning("The given %s and the peer %s have "
                                "duplicated server_id %s", server,
                                peer_server, peer_id)
                results["duplicate"] = peer_server
                results["pass"] = False
            else:
                _LOGGER.debug("The peer %s have a different server_id "
                              " %s", peer_server, peer_id)

        results["used_server_id_dict"] = server_ids
        if "pass" not in results.keys():
            results["pass"] = True

        return results

    def validate_schemas_gr_compliance(self, alt_server=None):
        """Validates the preexisting tables in the MySQL server, generating
        a report of the tables with engine than 'InnoDB', or not listed in
        GR_COMPLIANCE_SKIP_ENGINES. In addition the tables with 'InnoDB' engine
        that does not have a primary key as included in the report.

        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :return: A dictionary with key as 'reason' and lists of the
                 tables out of compliance as values, with reasons:
                     'engine_type'    unsupported engine
                     'primary_key'    lacking of primary key.
        :rtype: dict
        """
        _LOGGER.debug("checking whether existing tables comply to GR "
                      "requirements")

        server = self._get_server(alt_server)
        res = server.exec_query("SELECT table_schema, table_name, engine "
                                "FROM information_schema.tables "
                                "WHERE engine NOT IN {0} AND table_schema "
                                "NOT IN {1}"
                                "".format(GR_COMPLIANCE_SKIP_ENGINES,
                                          GR_COMPLIANCE_SKIP_SCHEMAS))

        # Store Result
        results = {
            "engine_type": [],
            "primary_key": []
        }
        if res:
            results["engine_type"] = res
            results["pass"] = False

        compliance_qry = (
            "SELECT t.table_schema, t.table_name "
            "FROM information_schema.tables t "
            "    LEFT JOIN information_schema.columns c "
            "    ON t.table_schema = c.table_schema "
            "        AND t.table_name = c.table_name "
            "WHERE t.table_type = 'BASE TABLE' "
            "    AND t.table_schema NOT IN {0} "
            "GROUP BY t.table_schema, t.table_name "
            "HAVING sum(if(c.column_key='PRI', 1, 0)) = 0"
            "".format(GR_COMPLIANCE_SKIP_SCHEMAS)
        )
        res = server.exec_query(compliance_qry)
        if res:
            results["primary_key"] = res
            results["pass"] = False

        if "pass" not in results.keys():
            results["pass"] = True

        return results

    def check_mts_compatibility(self, alt_server=None):
        """Check the Multi-Threaded Slave settings.

        Check the compatibility of the Multi-Threaded Slave (MTS) settings to
        use with group replication.

        If MTS is enabled (slave_parallel_workers > 0) then GR requires
        slave_parallel_type=LOGICAL_CLOCK and slave_preserve_commit_order=1
        (ON).

        :param alt_server: An alternative server instance to use.
        :type alt_server:  Server instance.
        """
        results = {'pass': True}
        server = self._get_server(alt_server=alt_server)
        # Check if MTS is enabled ( > 0)
        slave_p_workers = server.select_variable('slave_parallel_workers')
        if slave_p_workers and int(slave_p_workers) > 0:
            # MTS is enabled, then we need to check compatible settings.
            p_type = server.select_variable('slave_parallel_type')
            cmt_order = server.select_variable('slave_preserve_commit_order')
            # slave_parallel_type must be 'LOGICAL_CLOCK'
            if p_type.upper() != 'LOGICAL_CLOCK':
                results['slave_parallel_type'] = (False, 'LOGICAL_CLOCK',
                                                  p_type)
                results['pass'] = False
            # slave_preserve_commit_order must be '1' (ON)
            if cmt_order.upper() != '1':
                # slave_preserve_commit_order value is converted to 'OFF'
                # if not '1', in order to be able to be updated later
                # (if '1' is used for the SET command instead of 'OFF' it will
                # fail).
                results['slave_preserve_commit_order'] = (False, 'ON',
                                                          'OFF')
                results['pass'] = False

        return results

    def validate_peer_variables(self, peer_values, alt_server=None):
        """Validates a variable value is the same used in the given peer.

        :param peer_values: A dictionary with the required values to check.
                              peer   A server that is already member of a GR.
                              peer_variables  list of variables to check.
        :type peer_values:  dict
        :param alt_server:  An alternative server instance to use.
        :type alt_server:   Server instance.

        :return: a dictionary with the results, including the key pass with
                 True if the hash name used is the same in both servers.
        :rtype:  dict
        """
        # Get the server to verify
        server = self._get_server(alt_server)

        # store results
        results = {}
        missing = {}
        valid = True

        peer_server = peer_values["peer"]
        peer_variables = peer_values["peer_variables"]
        for peer_var in peer_variables:
            _LOGGER.debug("Checking option: '%s' ", peer_var)
            peer_var_value = peer_server.select_variable(peer_var, "global")
            try:
                cur_val = server.select_variable(peer_var, "global")
            except GadgetQueryError as err:
                _LOGGER.warning("Could not get the value of option %s, "
                                "cause: %s", peer_var, err)
                # the option needs to be set:
                missing[peer_var] = peer_var_value
                valid = False
            else:
                # values match
                if cur_val in peer_var_value:
                    _LOGGER.debug('expected value: %s found', cur_val)
                    res = (True, peer_var_value, cur_val)
                # values differ
                else:
                    _LOGGER.debug('expected value: %s differs from %s',
                                  peer_var_value, cur_val)
                    res = (False, peer_var_value, cur_val)
                    valid = False
                # save variable result
                results[peer_var] = res

        if missing:
            results["missing"] = missing
        results["pass"] = valid

        return results
