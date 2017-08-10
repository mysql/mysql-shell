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
This module contains common operation to manage Group Replication
"""
import collections
import datetime
import logging
import os
import socket

from mysql_gadgets import MIN_MYSQL_VERSION
from mysql_gadgets.exceptions import (GadgetDBError, GadgetError,
                                      GadgetQueryError,)
from mysql_gadgets.common.format import get_max_display_width
from mysql_gadgets.common.req_checker import (ALL_OF,
                                              DEFAULT,
                                              CONFIG_SETTINGS,
                                              ONE_OF,
                                              OPTION_PARSER,
                                              NOT_IN,
                                              PEER_SERVER_VARIABLES,
                                              RequirementChecker,
                                              SERVER_ID,
                                              SERVER_VARIABLES,
                                              SERVER_VERSION,
                                              USER_PRIVILEGES,
                                              MTS_SETTINGS)
from mysql_gadgets.common.config_parser import MySQLOptionsParser
from mysql_gadgets.common.server import get_server, generate_server_id
from mysql_gadgets.common.user import (change_user_privileges, parse_user_host,
                                       User,)

# Get common logger
_LOGGER = logging.getLogger(__name__)
# Four Column formatter
_MAX_WIDTH = get_max_display_width()
# The table looks strange if its too big _MAX_WIDTH > 150, remove the size
# used by (7) spaceces used as separator between columns.
_MAX_TABLE_WIDTH = _MAX_WIDTH - 7 if _MAX_WIDTH < 150 else 150
# Column aprox distribution using percents of the width of the console.
# 45, 22, 22, 8 = 100%
# Option Name column
_OP_C1_WIDTH = int(_MAX_TABLE_WIDTH * 0.45) if _MAX_WIDTH < 85 else 40
# Size of the columns
_C1, _C2, _C3, _C4 = (_OP_C1_WIDTH, int(_MAX_TABLE_WIDTH * 0.22),
                      int(_MAX_TABLE_WIDTH * 0.22),
                      int(_MAX_TABLE_WIDTH * 0.08))

_FOUR_COLS = "{{0:<{}}}  {{1:<{}}}  {{2:<{}}}  {{3:<{}}}".format(_C1, _C2,
                                                                 _C3, _C4)

_TWO_COLS = "{{0:<{}}}  {{1:<{}}}".format(_C1, _C2)
DISABLED_STORAGE_ENGINES = "disabled_storage_engines"

AUTO_INCREMENT_INCREMENT = "auto_increment_increment"
AUTO_INCREMENT_OFFSET = "auto_increment_offset"

GR_PLUGIN_NAME = "group_replication"
GR_ALLOW_LOCAL_LOWER_VERSION_JOIN = (
    "group_replication_allow_local_lower_version_join")
GR_AUTO_INCREMENT_INCREMENT = "group_replication_auto_increment_increment"
GR_BOOTSTRAP_GROUP = "group_replication_bootstrap_group"
GR_COMPONENTS_STOP_TIMEOUT = "group_replication_components_stop_timeout"
GR_GCS_ENGINE = "group_replication_gcs_engine"
GR_GROUP_NAME = "group_replication_group_name"
GR_GROUP_SEEDS = "group_replication_group_seeds"
GR_IP_WHITELIST = "group_replication_ip_whitelist"
GR_LOCAL_ADDRESS = "group_replication_local_address"
GR_PIPELINE_TYPE_VAR = "group_replication_pipeline_type_var"
GR_RECOVERY_COMPLETE_AT = "group_replication_recovery_complete_at"
GR_RECOVERY_RECONNECT_INTERVAL = (
    "group_replication_recovery_reconnect_interval")
GR_RECOVERY_RETRY_COUNT = "group_replication_recovery_retry_count"
GR_RECOVERY_SSL_CA = "group_replication_recovery_ssl_ca"
GR_RECOVERY_SSL_CAPATH = "group_replication_recovery_ssl_capath"
GR_RECOVERY_SSL_CERT = "group_replication_recovery_ssl_cert"
GR_RECOVERY_SSL_CIPHER = "group_replication_recovery_ssl_cipher"
GR_RECOVERY_SSL_CRL = "group_replication_recovery_ssl_crl"
GR_RECOVERY_SSL_CRLPATH = "group_replication_recovery_ssl_crlpath"
GR_RECOVERY_SSL_KEY = "group_replication_recovery_ssl_key"
GR_RECOVERY_SSL_VERIFY_SERVER_CERT = (
    "group_replication_recovery_ssl_verify_server_cert")
GR_RECOVERY_USE_SSL = "group_replication_recovery_use_ssl"
GR_SINGLE_PRIMARY_MODE = "group_replication_single_primary_mode"
GR_START_ON_BOOT = "group_replication_start_on_boot"
GR_SSL_MODE = "group_replication_ssl_mode"
# GR SSL modes
GR_SSL_REQUIRED = "REQUIRED"
GR_SSL_DISABLED = "DISABLED"
GR_SSL_VERIFY_CA = "VERIFY_CA"
GR_SSL_VERIFY_IDENTITY = "VERIFY_IDENTITY"
# Deprecated variables
GR_PEER_ADDRESSES = "group_replication_peer_addresses"
GR_RECOVERY_USER = "group_replication_recovery_user"
GR_RECOVERY_PASSWORD = "group_replication_recovery_password"

HAVE_SSL = "have_ssl"

# option file section 'mysqld'
MYSQLD_SECTION = "mysqld"
# Group Replication connection status table
REP_CONN_STATUS_TABLE = "performance_schema.replication_connection_status"
# Group Replication group members table
REP_GROUP_MEMBERS_TABLE = "performance_schema.replication_group_members"
# Group Replication member stats table
REP_MEMBER_STATS_TABLE = "performance_schema.replication_group_member_stats"
# Group Replication applier status table
REP_APPLIER_STATUS_BY_WORKER = ("performance_schema."
                                "replication_applier_status_by_worker")

TRANSACTION_WRITE_SET_EXTRACTION = "transaction_write_set_extraction"

_ERROR_GR_NOT_LOAD_ON_PEER = ("Unable to get the group replication {var_name}"
                              " from the given peer server: {server}, the GR "
                              "plugin is not loaded.")

_GR_PLUGIN_DISABLED_REQ_UNINSTALL = ("The group_replication plugin for %s "
                                     "is DISABLED, uninstall is required "
                                     "prior to enable it.")

# IMPORTANT NOTE: The "restart the server" string is caught by the Shell to
# determine if the server needs to be restarted or not. Therefore any error
# that requires a restart of the server MUST include that specific string.
_ERROR_GR_PLUGIN_DISABLED = ("The group_replication plugin for {0} is "
                             "DISABLED and it cannot be activated online "
                             "without access to the option file used to start "
                             "the server. Please restart the server making "
                             "sure the group replication plugin is ACTIVE.")
_ERROR_RESTART_SERVER = ("Please restart the server {0} with the updated "
                         "options file and try again.")
_ERROR_UPDATE_AND_RESTART = ("Please change the configuration values on your "
                             "options file, restart the server and try again.")
_ERROR_SERVER_VARIABLES = (
    "Some active options on server {0} are incompatible with Group "
    "Replication.\n{1}")

# Commands
START = "START"
JOIN = "JOIN"
HEALTH = "HEALTH"
LEAVE = "LEAVE"
DRY_RUN = "DRY-RUN"

_HEX_NUM = "[a-z|A-Z|0-9]"
REX_UUID = (r"{0}{{8}}\-{0}{{4}}\-{0}{{4}}\-{0}{{4}}\-{0}{{12}}"
            "".format(_HEX_NUM))

OPT_SKIP_CHECK_GR_SCHEMA_COMPLIANCE = "--allow-non-compatible-tables"

REQ_GR_PLUGIN_VER = ".".join(str(i) for i in MIN_MYSQL_VERSION)

# Loose prefix for options in the defaults file.
LOOSE_PREFIX = "loose_{0}"

# List of options that requires to append values to current value.
APPEND_VALUE_OPTIONS = [GR_GROUP_SEEDS]

# a set of GR options to read from the defaults file and setup in the server.
DEFAULTS_FILE_OPTIONS = frozenset({GR_IP_WHITELIST,
                                   GR_GROUP_NAME,
                                   GR_GROUP_SEEDS,
                                   GR_LOCAL_ADDRESS,
                                   GR_SINGLE_PRIMARY_MODE})

EQUIVALENT_OPTION_VALUES = {
    "ON": {"ON", "1"},
    "1": {"ON", "1"},
    "0": {"OFF", "0"},
    "OFF": {"OFF", "0"},

}

# Required configuration on MySQL server (variables)
GR_REQUIRED_CONFIG = {
    # Options related to binlog
    # log_bin option can be set to a file, so we check is not 0 or OFF
    "log_bin": {ONE_OF: ("1", "ON")},
    "binlog_format": {ONE_OF: ("ROW",)},
    "binlog_checksum": {ONE_OF: ("NONE",)},

    # Options related with GTids
    "gtid_mode": {ONE_OF: ("ON",)},
    "log_slave_updates": {ONE_OF: ("ON", "1")},
    "enforce_gtid_consistency": {ONE_OF: ("ON", "1")},

    # WL#9426 Activates the automatic election. This means that there
    #   will only be a primary server that takes incoming updates
    #   and all others in the group will be secondaries. On
    #   primary failures, the group will elect the new primary
    #   from the pool of secondaries. Default: TRUE.
    # GR_SINGLE_PRIMARY_MODE: {ONE_OF: ("ON", "1")},

    # Options related with persistence of replication data.
    "master_info_repository": {ONE_OF: ("TABLE",)},
    "relay_log_info_repository": {ONE_OF: ("TABLE",)},

    # Hash used for extract what writes were made during a transaction.
    TRANSACTION_WRITE_SET_EXTRACTION: {
        ONE_OF: ("XXHASH64", "2", "MURMUR32", "1")},
}
# List of server dynamic variables (do not require restart to be updated).
# NOTE: gtid_mode and enforce_gtid_consistency are dynamic variables their
# value cannot be changed on the fly directly from OFF to ON,

# NOTE: master_info_repository and relay_log_info_repository are also dynamic
# variables but they cannot be changed online if any replication threads are
# running.
DYNAMIC_SERVER_VARS = frozenset({"binlog_format",
                                 # "gtid_mode",
                                 # "enforce_gtid_consistency",
                                 # "master_info_repository",
                                 # "relay_log_info_repository",
                                 "binlog_checksum"})

# Required configuration on MySQL option file
GR_REQUIRED_OPTIONS = {
    # Options related to binlog
    # log_bin option can be set to a file, so we check is not 0 or OFF
    # The "<no value>" tag is used to distinguish the value None when the
    # value is printed, and from the tag <not set> that means the option does
    # not exist.
    "log_bin": {NOT_IN: ("OFF", "0", "<not set>"), DEFAULT: "<no value>"},
    "binlog_format": {ONE_OF: ("ROW",)},
    "binlog_checksum": {ONE_OF: ("NONE",)},

    # options related to network
    "bind-address": {ONE_OF: ("*", "<not set>"), DEFAULT: "*"},

    # Options related with GTids
    "gtid_mode": {ONE_OF: ("ON", "1")},
    "log_slave_updates": {ONE_OF: ("ON", "1")},
    "enforce_gtid_consistency": {ONE_OF: ("ON", "1")},

    # WL#9426 Activates the automatic election. This means that there
    #   will only be a primary server that takes incoming updates
    #   and all others in the group will be secondaries. On
    #   primary failures, the group will elect the new primary
    #   from the pool of secondaries. Default: TRUE.
    # GR_SINGLE_PRIMARY_MODE: {ONE_OF: ("ON", "1")},

    # Options related with persistence of replication data.
    "master_info_repository": {ONE_OF: ("TABLE",)},
    "relay_log_info_repository": {ONE_OF: ("TABLE",)},
    "server_id": {NOT_IN: ("0",), DEFAULT: generate_server_id()},

    # Hash used for extract what writes were made during a transaction.
    TRANSACTION_WRITE_SET_EXTRACTION: {
        ONE_OF: ("XXHASH64", "2", "MURMUR32", "1")},
    # Not supported engines by GR.
    DISABLED_STORAGE_ENGINES: {
        ALL_OF: ("MyISAM", "BLACKHOLE", "FEDERATED", "CSV", "ARCHIVE",)},

    # Not necessary to set plugin-load = group_replication.so/ddl on the
    # configuration file  since the the check, start-replicaset
    # and join-replicaset operations all install the group_replication plugin.
    # After the plugin is installed, it is automatically loaded henceforth
    # even if the server is shutdown or restarted until the uninstall command
    # for the plugin is issued.

}

DEFAULT_REQ_DICT = {
    SERVER_VERSION: REQ_GR_PLUGIN_VER,
}

ERROR_PEERS_VARIABLE = ("The value '{0}' in variable '{1}' differs from the "
                        "value '{2}' in the peer server {3} and it must be "
                        "equals on all the members of the group.")

PEER_VARIABLES = (TRANSACTION_WRITE_SET_EXTRACTION, )

INNODB_RQD_MSG = ("Group Replication requires tables to use InnoDB and "
                  "have a PRIMARY KEY or PRIMARY KEY Equivalent (non-null "
                  "unique key). Tables that do not follow these "
                  "requirements will be readable but not updateable "
                  "when used with Group Replication. "
                  "If your applications make updates (INSERT, UPDATE or "
                  "DELETE) to these tables, ensure they use the InnoDB "
                  "storage engine and have a PRIMARY KEY or PRIMARY KEY "
                  "Equivalent.")
SKIP_SCHEMA_MSG = ("You can retry this command with the {0} option "
                   "if you'd like to enable Group Replication ignoring "
                   "this warning.".format(OPT_SKIP_CHECK_GR_SCHEMA_COMPLIANCE))


def _format_table_results(dict_result, verbose=False):
    """Formats a table of the given dictionary results.

    The dict must have the format:
        Option_name: (result_of_comparison, expected_result, current_value)

    :param dict_result: A dict with the checking results
    :type dict_result:  dict
    :param verbose:  If the output should be verbose, by default False.
    :type verbose:   bool.

    @return: A list the lines that compose the table with the results
    @rtype: list
    """
    # Formatted table of results.
    table = []

    server_vars = dict_result.copy()
    if "pass" in server_vars.keys():
        server_vars.pop("pass")
    col_n = list(server_vars.keys())
    col_n.sort()
    col_n.insert(0, "Variable name")

    rows = []
    for var in col_n[1:]:
        res, exp, cur_val = server_vars[var]
        # Show passed options only if verbose output or variable value is
        # not the one expected
        if verbose:
            rows.append(_FOUR_COLS.format(var, exp, cur_val, "PASS" if res
                                          else "FAIL"))
        elif not res:
            rows.append(_FOUR_COLS.format(var, exp, cur_val, "PASS" if res
                                          else "FAIL"))
    for row in rows:
        table.append(row)
    if table:
        table.insert(0, _FOUR_COLS.format("Option name", "Required Value",
                                          "Current Value", "Result"))
        table.insert(1, _FOUR_COLS.format("-" * _C1, "-" * _C2, "-" * _C3,
                                          "-" * _C4))
    return table


def _print_check_config_results(missing):
    """Prints the missing configuration variables from checking the group
    replication requirements on the options file.

    :param missing: A dict with the missing configuration variables
    :type missing:  dict
    """
    # Log SERVER_VARIABLES results
    if missing is not None and missing:
        server_vars = missing.copy()

        col_n = list(server_vars.keys())
        col_n.sort()
        col_n.insert(0, "Option name")

        rows = []
        for var in col_n[1:]:
            cur_val = server_vars[var]
            rows.append(_TWO_COLS.format(var, cur_val))

        for row in rows:
            _LOGGER.info(row)

        _LOGGER.info("")


def _print_check_user_results(user_priv_res, hostname):
    """Prints the results from checking the user replication requirements

    :param user_priv_res:  A dict with the check results
    :type user_priv_res:   dict
    :param hostname:  The host name where the check was performed.
    :type hostname:   string
    """
    # Log USER_PRIVILEGES results
    if not user_priv_res["pass"]:
        user_privs = user_priv_res.copy()
        user_privs.pop("pass")
        rows = []
        for usr in user_privs:
            if 'NO EXISTS!' in user_privs[usr]:
                rows.append(
                    "The user {0} does not exists on {1} and requires to be "
                    "created.".format(usr, hostname))
            elif user_privs[usr]:
                # List of missing privileges is not empty.
                rows.append(
                    "The user {0} does not have the required privileges: {1}"
                    " on {2}.".format(usr, user_privs[usr], hostname))
        for row in rows:
            _LOGGER.info(row)


def _print_check_unique_id_results(server_id_res):
    """Prints the results from checking the server_id

    :param server_id_res: A dict with the check results
    :type server_id_res:  dict
    """
    if server_id_res["pass"]:
        _LOGGER.info("The server_id is valid.")
    else:
        duplicate = server_id_res.get("duplicate", None)
        if duplicate is not None:
            _LOGGER.info("The server_id %s is already used by peer %s",
                         duplicate.select_variable("server_id"),
                         duplicate)
        else:
            _LOGGER.info("The server_id is not valid.")


def _print_check_server_version_results(server_ver_res):
    """Prints the results from checking the group replication requirements

    :param server_ver_res:  A dict with the check results
    :type server_ver_res:   dict
    """
    # Log SERVER_VERSION results
    server_version = ".".join([str(ver) for ver in
                               server_ver_res[SERVER_VERSION]])
    if server_ver_res["pass"]:
        msg = ("Server is {0}".format(server_version))
    else:
        msg = ("The version of the server is {0} and does not meet the "
               "required MySQL server version {1}".format(server_version,
                                                          REQ_GR_PLUGIN_VER))
    _LOGGER.info(msg)
    _LOGGER.info("")


def set_bootstrap(server, dry_run=False):
    """Sets the group_replication_bootstrap_group variable to 1

    :param server:  A server with group replication plugin loaded
    :type server:   Server instance (mysql_gadgets.common.server).
    :param dry_run: If true no actions will affect the given server.
    :type dry_run:  bool.
    """
    if not dry_run:
        server.exec_query("SET GLOBAL {0} = 1".format(GR_BOOTSTRAP_GROUP))


def unset_bootstrap(server, dry_run=False):
    """Sets the group_replication_bootstrap_group variable to 0

    :param server:   A server with group replication plugin loaded
    :type server:   Server instance (mysql_gadgets.common.server).
    :param dry_run: If true no actions will affect the given server.
    :type dry_run:  bool.
    """
    if not dry_run:
        server.exec_query("SET GLOBAL {0} = 0".format(GR_BOOTSTRAP_GROUP))


def get_gr_name_from_peer(peer_server):
    """Retrieves the GR name from where the given server belongs.
    :param peer_server: A server that is already member of a replication group.
    :type peer_server:  Server instance (mysql_gadgets.common.server).

    :raise GadgetError: If the given peer_server has not loaded the Group
                        Replication plugin.

    :return:  The group replication name (UUID) of the replication group from
              where the given peer_server belongs.
    :rtype:   string
    """
    if peer_server.is_plugin_installed(GR_PLUGIN_NAME):
        res = peer_server.exec_query("SELECT GROUP_NAME FROM {0} WHERE "
                                     "CHANNEL_NAME='group_replication_applier'"
                                     "".format(REP_CONN_STATUS_TABLE))
        return "'{0}'".format(res[0][0]) if res else "''"
    else:
        raise GadgetError(
            _ERROR_GR_NOT_LOAD_ON_PEER.format(var_name="group name",
                                              server=peer_server))


def get_gr_local_address_from(peer_server):
    """Retrieves the group_replication_local_address variable value from the
    given sever.
    :param peer_server: A server that is already member of a replication group.
    :type peer_server:  Server instance (mysql_gadgets.common.server).

    :raise GadgetError: If the given peer_server has not loaded the Group
                        Replication plugin.

    :return:  The group replication name (UUID) of the replication group from
              where the given peer_server belongs.
    :rtype:   string
    """
    if peer_server.is_plugin_installed(GR_PLUGIN_NAME):
        var_value = peer_server.select_variable(GR_LOCAL_ADDRESS, "global")
        local_address = "'{0}'".format(var_value).replace("localhost",
                                                          peer_server.host)
        return local_address.replace("127.0.0.1", peer_server.host)
    else:
        raise GadgetError(
            _ERROR_GR_NOT_LOAD_ON_PEER.format(var_name="local address",
                                              server=peer_server))


def get_gr_variable_from_peer(peer_server, var_name, use_synonym=True):
    """Retrieves the given GR variable value from the given peer sever.

    :param peer_server: A server that is already member of a replication group.
    :type peer_server:  Server instance (mysql_gadgets.common.server).
    :param var_name: The variable name to retrieve from the peer server.
    :type var_name:  string
    :param use_synonym: Indicates if '1' or '0' should be return as 'ON' or
                    'OFF' respectively, by default True.
    :type use_synonym:  boolean

    :raise GadgetError: If the given peer_server has not loaded the Group
                        Replication plugin or it was unable to retrieve the
                        variable value.

    :return:  The value of the group replication plugin's variable.
    :rtype:   string
    """
    _LOGGER.debug("Trying to retrieve group replication variable: %s from "
                  "peer server: %s.", var_name, peer_server)
    if peer_server.is_plugin_installed(GR_PLUGIN_NAME):
        var_value = peer_server.select_variable(var_name, "global")
        _LOGGER.debug("Retrieved variable: %s from peer server: %s with "
                      "value: '%s'.", var_name, peer_server, var_value)
        if use_synonym and var_value in ['0', '1']:
            var_value = 'ON' if var_value == '1' else 'OFF'
        return "'{0}'".format(var_value)
    else:
        raise GadgetError(
            _ERROR_GR_NOT_LOAD_ON_PEER.format(var_name=var_name,
                                              server=peer_server))


def setup_gr_config(server, gr_config_vars, disable_binlog=True,
                    dry_run=False):
    """Sets the GR config variables in the given server.

    :param server:         A server where to try to load the GR plugin
    :type server:          Server instance (mysql_gadgets.common.server).
    :param gr_config_vars: A dictionary with the variable and their values to
                           set in the given server.
    :type gr_config_vars:  dict.
    :param disable_binlog: If the binary log should be disabled during the
                           operation, by default True.
    :type disable_binlog:  bool.
    :param dry_run:        If true no actions will affect the given server.
    :type dry_run:         bool.
    """
    # setup Variables
    msg = "Setting Group Replication variables"
    _LOGGER.debug(msg)

    for var in gr_config_vars.keys():
        if dry_run:
            _LOGGER.debug("The global variable %s will be set as %s",
                          var, gr_config_vars[var])
        else:
            msg = ("  {0} = {1}".format(var, gr_config_vars[var]))
            _LOGGER.debug(msg)
            try:
                if disable_binlog:
                    server.exec_query("SET SQL_LOG_BIN=0")
                server.exec_query("SET GLOBAL {0} = {1}"
                                  "".format(var, gr_config_vars[var]))

            except GadgetQueryError as db_err:
                raise GadgetError("An error occurred while setting variable "
                                  "\n{0}: {1}".format(msg, db_err.errmsg))
            # Restore binlog if it was disabled.
            finally:
                if disable_binlog:
                    server.exec_query("SET SQL_LOG_BIN=1")


def get_group_uuid_name(server):
    """Returns a new UUID for the GR group name, using the given server.

    :return: a new UUID for the GR group name.
    :rtype: string
    """
    res = server.exec_query("SELECT UUID()")
    return "'{0}'".format(res[0][0])


def install_plugin(server, dry_run=False):
    """Installs the GROUP_REPLICATION plugin

    :param server:  A server where to try to install the GR plugin
    :type server:   Server instance (mysql_gadgets.common.server).
    :param dry_run: If true no actions will affect the given server.
    :type dry_run:  bool.

    :raise GadgetError: If the GR plugin could not be installed/loaded
    """
    msg = ("* Verifying Group Replication plugin for server "
           "{0} ...".format(server))
    _LOGGER.info(msg)
    if dry_run:
        if server.is_plugin_installed(GR_PLUGIN_NAME):
            _LOGGER.info("Group Replication plugin: loaded")
        else:
            _LOGGER.info("Group Replication plugin: Not loaded")
    else:
        super_read_only = server.select_variable("super_read_only", 'global')
        if (int(super_read_only)):
            server.set_variable("super_read_only", 'OFF','global')

        server.install_plugin(GR_PLUGIN_NAME)

        if (int(super_read_only)):
            server.set_variable("super_read_only", 'ON','global')

def uninstall_plugin(server, dry_run=False):
    """Uninstalls the GROUP_REPLICATION plugin

    :param server:  A server where to try to install the GR plugin
    :type server:   Server instance (mysql_gadgets.common.server).
    :param dry_run: If true no actions will affect the given server.
    :type dry_run:  bool.

    :raise GadgetError: If the GR plugin could not be installed/loaded
    """
    _LOGGER.info("* Uninstalling Group Replication plugin for server "
                 "%s ...", server)
    if dry_run:
        if server.is_plugin_installed(GR_PLUGIN_NAME):
            _LOGGER.info("Group Replication plugin: loaded")
        else:
            _LOGGER.info("Group Replication plugin: Not loaded")
    else:
        server.uninstall_plugin(GR_PLUGIN_NAME)


def start_gr_plugin(server, dry_run=False):
    """Starts the GROUP_REPLICATION plugin.

    :param server:  A server where to try to start the GR plugin.
    :type server:   Server instance (mysql_gadgets.common.server).
    :param dry_run: If true no actions will affect the given server.
    :type dry_run:  bool.
    """
    _LOGGER.debug("")
    _LOGGER.debug("* Starting Group Replication plugin...")
    if dry_run:
        _LOGGER.debug("skipped by --dry-run option.")
    else:
        server.start_plugin(GR_PLUGIN_NAME)


def stop_gr_plugin(server):
    """Stops the GROUP_REPLICATION plugin

    :param server: A server where to try to stop the GR plugin.
    :type server: Server instance (mysql_gadgets.common.server).
    """
    _LOGGER.info("")
    _LOGGER.info("* Stopping Group Replication plugin...")

    server.stop_plugin(GR_PLUGIN_NAME)


def _grant_slave_replication(server, recovery_user, replication_host):
    """Grants REPLICATION SLAVE to the recovery user
    This operation is always done with binary log turned OFF; otherwise this
    will be logged and can create transaction conflicts between the
    replication servers.

    :param server: The server instance where to grant the privilege
    :type server:  Server
    :param recovery_user: The user name used for recovery
    :type recovery_user:  string
    :param replication_host: The host name associated to the replication user.
    :type replication_host:  string

    :raise GadgetError: If the server user does not have the privileges
                        to grant REPLICATION SLAVE over replication user.
    """

    if replication_host[0] == "'":
        replication_host = replication_host[1:-1]
    change_user_privileges(server, recovery_user, replication_host,
                           grant_list=["REPLICATION SLAVE"],
                           disable_binlog=True,)

    _LOGGER.info("Successfully granted REPLICATION SLAVE to user: %s@%s",
                 recovery_user, replication_host)


def do_change_master(server, rpl_user_settings, disable_binlog=True,
                     dry_run=False):
    """Setup the recovery user and password with a CHANGE MASTER statement.

    :param server:  The server instance where to run the statement
    :type server:   Server
    :param rpl_user_settings: a dictionary with the recovery_user and
                    rep_user_passwd to be used in the change master statement.
    :type rpl_user_settings:   dict
    :param disable_binlog: Turn of the binary while creating the user.
    :type disable_binlog: Boolean, if True turn off the binary log
    :param dry_run: If true no actions will affect the given server.
    :type dry_run:  bool.
    """
    recovery_user = rpl_user_settings["recovery_user"]
    rep_user_passwd = rpl_user_settings["rep_user_passwd"]

    # Log the change master statement is about to be run
    _LOGGER.info("* Running change master command")

    if dry_run:
        _LOGGER.info("The CHANGE MASTER command will set MASTER_USER as "
                     "%s", recovery_user)
    else:
        try:
            if disable_binlog:
                server.toggle_binlog(action="disable")

            query = ("CHANGE MASTER TO MASTER_USER = '{0}', "
                     "MASTER_PASSWORD = '{1}' FOR CHANNEL "
                     "'group_replication_recovery';")
            server.exec_query(query.format(recovery_user, rep_user_passwd),
                              query_to_log=query.format(recovery_user,
                                                        "******"))
        except GadgetQueryError as db_err:
            raise GadgetError("An error occurred while running change "
                              "master:\n{0}".format(db_err.errmsg))
        # Restore binlog if it was disabled.
        finally:
            if disable_binlog:
                server.toggle_binlog(action="enable")


def get_member_state(server):
    """Retrieves the GR member state of the given server.

    :param server:  The server instance where to run the statement
    :type server:   Server

    :return: The member state of the server from replication_group_members.
    :rtype: string.
    """
    try:
        res = server.exec_query("SELECT MEMBER_STATE FROM {0} as m JOIN {1} "
                                "as s on m.MEMBER_ID = s.MEMBER_ID "
                                "AND m.MEMBER_ID = @@server_uuid"
                                "".format(REP_GROUP_MEMBERS_TABLE,
                                          REP_MEMBER_STATS_TABLE))
        if res:
            return res[0][0]
        else:
            return None
    except GadgetQueryError as err:
        if "SELECT command denied" in err.errmsg:
            _LOGGER.warning("The user %s does not have enough privileges to "
                            "verify server replication state", server.user)
        else:
            raise
    return None


def is_active_member(server):
    """Verifies if the given server is already a member of a GR group.

    :param server:  The server instance where to run the statement
    :type server:   Server

    :return: True if the server is not OFFLINE, otherwise false.
    :rtype: boolean.
    """
    state = get_member_state(server)
    if state is None or state == "OFFLINE":
        return False
    else:
        return True


def is_member_of_group(server, group_id=None):
    """Retrieves the GR member state of the given server.

    :param server:  The server to check membership of.
    :type server:   Server
    :param group_id:  Check if the group has this UUId by group name.
    :type group_id:   string

    :return: True if the server is member of a group (with group_id if given),
             otherwise false.
    :rtype: boolean.
    """
    try:
        res = server.exec_query("SELECT GROUP_NAME FROM {0} where "
                                "CHANNEL_NAME = 'group_replication_applier'"
                                "".format(REP_CONN_STATUS_TABLE))
    except GadgetError as err:
        _LOGGER.warning(err.errmsg)
        if "SELECT command denied" in err.errmsg:
            raise GadgetError("The user {0} does not have enough "
                              "privileges to verify server replication "
                              "state.".format(server.user))
        else:
            raise

    if res:
        if group_id:
            return res[0][0] == group_id
        else:
            return True
    else:
        return False


def update_option_file(opt_parser, missing_values, update_values,
                       skip_backup=False):
    """Attempts to update the given options file
    :param opt_parser:    The MySQL option parser to update
    :type opt_parser:     MySQLOptionsParser
    :param missing_values: A dictionary of variables names, and the values
                           to set in the option file
    :type missing_values:  dict
    :param update_values:  A dictionary of variables names, and the values
                           to update in the option file
    :type update_values:   dict
    :param skip_backup: if True, skip the creation of a backup file when
                        modifying the options file.
    :type skip_backup: bool
    :return: True in case the option file was updated, False if not.
    :rtype: boolean
    """
    # Dictionary should not be used as argument for logger, since it duplicates
    # backslash. Convert to string and convert \\ back to \.
    dic_msg = str(update_values)
    dic_msg = dic_msg.replace("\\\\", "\\")
    _LOGGER.debug("update_values %s", dic_msg)
    # Verify option parser can update file
    if not os.access(opt_parser.filename, os.W_OK):
        return False

    # Create the section if does not exist already
    if not opt_parser.has_section(MYSQLD_SECTION):
        _LOGGER.debug("Creating %s section", MYSQLD_SECTION)
        opt_parser.add_section(MYSQLD_SECTION)

    # Update
    for option, values in update_values.items():
        # if res is false, if needs to update value
        result, new_val, cur_val = values
        if result:
            _LOGGER.debug("On section %s, option %s has the expected value %s",
                          MYSQLD_SECTION, option, cur_val)
        else:
            _LOGGER.debug("On section %s updating option %s with old value %s "
                          "with new value %s", MYSQLD_SECTION, option,
                          cur_val, new_val)
            # MySQLOptionsParser requires the <no value> to be substituted by
            # None to set the variable with no value assigned to it.
            if new_val == '<no value>':
                new_val = None
            opt_parser.set(MYSQLD_SECTION, option, new_val)

    # Add missing options
    for option, value in missing_values.items():
        _LOGGER.debug("On section %s adding missing option %s with value %s",
                      MYSQLD_SECTION, option, value)
        # MySQLOptionsParser requires the <no value> to be substituted by
        # None to set the variable with no value assigned to it.
        if value == '<no value>':
            value = None
        opt_parser.set(MYSQLD_SECTION, option, value)

    # Write changes
    if not skip_backup:
        # Create a backup file
        # get timestamp value
        ts = datetime.datetime.today()
        # remove microseconds
        ts = ts.replace(microsecond=0)
        # get timestamp string removing the : and the - separators
        str_ts = ts.isoformat("-").replace(":", "").replace("-", "", 2)
        # calculate backup file name from original file name plus timestamp
        backup_file = "{0}.{1}".format(opt_parser.filename, str_ts)
    else:
        # don't create a backup file
        backup_file = None

    opt_parser.write(backup_file_path=backup_file)
    return True


def check_peer_variables(req_checker, error_msgs=None, verbose=False):
    """Checks the server variables that requires specific values for GR plugin

    :param req_checker: RequirementChecker instance used to run the tests.
    :type req_checker:  RequirementChecker
    :param error_msgs:  List of errors to which errors that occur during check
                        are appended.
    :type error_msgs:   list
    :param verbose:     If the output should be verbose, by default False.
    :type verbose:      bool.
    """
    if error_msgs is None:
        error_msgs = []
    # retrieve the requirement dictionary
    req_dict = req_checker.req_dict

    server_var_res = req_checker.validate_peer_variables(
        req_dict[PEER_SERVER_VARIABLES])

    test_result = "PASS" if server_var_res["pass"] else "FAIL"
    _LOGGER.info("* Comparing options compatibility with the group of the "
                 "given peer-instance... %s", test_result)

    missing = {}
    if "missing" in server_var_res.keys():
        missing = server_var_res.pop("missing")

    if server_var_res["pass"]:
        _LOGGER.info("Server configuration is compliant with current group "
                     "configuration.")
        res_table = "\n".join(_format_table_results(server_var_res, verbose) +
                              _format_table_results(missing, verbose))
        if res_table:
            _LOGGER.info(res_table)
    else:
        _LOGGER.info("Some options need to be changed.")

    # Update variables to be check in the options file
    if OPTION_PARSER in req_dict.keys():
        server_vars = server_var_res.copy()
        server_vars.pop("pass")
        new_vars = {}
        for var_name in server_vars.keys():
            res, exp_val, _ = server_vars[var_name]
            # res is True only if the exp_val is the expected
            if not res:
                new_vars[var_name] = {ONE_OF: (exp_val,)}
        for var_name in missing:
            exp_val = missing[var_name]
            new_vars[var_name] = {ONE_OF: (exp_val,)}
        # Update required config options in options file
        # NOTE: Dictionary should not be used as argument for logger, since it
        # duplicates backslash. Convert to string and convert \\ back to \.
        dic_msg = str(new_vars)
        dic_msg = dic_msg.replace("\\\\", "\\")
        _LOGGER.debug("Updating required options %s", dic_msg)
        req_dict[CONFIG_SETTINGS].update(new_vars)

    if not server_var_res["pass"]:
        msg = ("The configuration of server {0} is not compliant with the "
               "requirements to join the Group Replication group where the "
               "provided peer instance belongs."
               "".format(req_checker.server))
        # append error message
        error_msgs.append(msg)
        error_msgs.extend(_format_table_results(server_var_res, verbose))
        error_msgs.extend(_format_table_results(missing, verbose))
    return server_var_res


def check_compliance(req_checker, error_msgs=None):
    """Checks if existing tables uses InnoDB and have a Primary key,

    :param req_checker: RequirementChecker instance used to run the tests.
    :type req_checker:  RequirementChecker
    :param error_msgs:  List of errors to which errors that occur during check
                        are appended.
    :type error_msgs:   list
    """
    if error_msgs is None:
        error_msgs = []
    try:
        compliance = req_checker.validate_schemas_gr_compliance()
    except GadgetError as err:
        error_msgs.append(err.errmsg)
    else:
        if compliance:
            bad_engine = compliance["engine_type"]
            bad_pk = compliance["primary_key"]
            test_result = "PASS" if not bad_engine and not bad_pk else "FAIL"
        else:
            test_result = "SKIP"
        _LOGGER.info("* Checking compliance of existing tables... %s",
                     test_result)

        if compliance:
            if bad_engine:
                _LOGGER.error("%i table(s) do not use InnoDB", len(bad_engine))
                for schema, table, engine in bad_engine:
                    _LOGGER.info("\t%s.%s (%s)", schema, table, engine)
                _LOGGER.info("")

            if bad_pk:
                _LOGGER.error(
                    "%i table(s) do not have a Primary Key or Primary Key "
                    "Equivalent (non-null unique key).", len(bad_pk))
                _LOGGER.info("\t%s",
                             ", ".join(["{0}.{1}".format(s, t) for s, t
                                        in bad_pk]))
                _LOGGER.info("")

            if bad_engine or bad_pk:
                error_msgs.append("Non-compatible tables found in database.")
                _LOGGER.info(INNODB_RQD_MSG)
                _LOGGER.info(SKIP_SCHEMA_MSG)
                _LOGGER.info("")


def check_user_privileges(req_checker, rpl_settings, dry_run=False,
                          verbose=False, error_msgs=None):
    """Checks the user privileges

    :param req_checker: RequirementChecker instance used to run the tests.
    :type req_checker:  RequirementChecker
    :param rpl_settings: Replication settings; replication user, user pw,
                        recovery user.
    :type rpl_settings: dict.
    :param dry_run:     If true no actions will affect the given server.
    :type dry_run:      bool.
    :param verbose:     If the output should be verbose.
    :type verbose:      bool.
    :param error_msgs:  List of errors to which errors that occur during check
                        are appended.
    :type error_msgs:   list
    """
    if error_msgs is None:
        error_msgs = []
    replication_user = rpl_settings["replication_user"]
    replication_host = rpl_settings["host"]
    recovery_user = rpl_settings["recovery_user"]
    rep_user_passwd = rpl_settings["rep_user_passwd"]

    # Always create a user without REQUIRED SSL. To change it later if needed,
    # set the require_ssl variable based on the proper condition, for example:
    # = rpl_settings.get("ssl_mode", GR_SSL_REQUIRED) != GR_SSL_DISABLED
    require_ssl = False

    req_dict = req_checker.req_dict
    server = req_checker.server
    # tests fails by default.
    test_result = "FAIL"

    try:
        user_priv_res = req_checker.check_user_privileges(
            req_dict[USER_PRIVILEGES])

    except GadgetError as err:
        error_msgs.append(err.errmsg)

    else:
        _print_check_user_results(user_priv_res, str(server))

        server_user = "{0}@'{1}'".format(server.user, server.host)

        # in case it passes we are done, rpl user exist and privileges are OK
        if user_priv_res["pass"]:
            # Dictionary should not be used as argument for logger, since it
            # duplicates backslash. Convert to string and convert \\ back to \.
            dic_msg = str(user_priv_res)
            dic_msg = dic_msg.replace("\\\\", "\\")
            _LOGGER.debug("The replication_user exists and has the required "
                          "privileges: %s.", dic_msg)
            test_result = "PASS"

        # check if usr can be created
        elif 'NO EXISTS!' in user_priv_res[replication_user]:
            _LOGGER.debug("Replication user %s does not exist.",
                          replication_user)

            # have privileges to create the replication user
            if "CREATE USER" not in user_priv_res[server_user] and \
               "REPLICATION SLAVE" not in user_priv_res[server_user] and \
               "SUPER" not in user_priv_res[server_user]:
                _LOGGER.debug("The user %s has the right privileges to create"
                              " the replication user account.", server.user)
                # Create the user with the resolved hostname replication_host
                rep_user_obj = User(server,
                                    "'{0}'@'{1}'".format(recovery_user,
                                                         replication_host),
                                    rep_user_passwd, verbose)
                # Also create a localhost account
                # Note: seems needed by GR plugin and helps join command to
                # verify the existency of the replication user account.
                rep_user_loc = User(server,
                                    "'{0}'@'localhost'".format(recovery_user),
                                    rep_user_passwd, verbose)

                if not dry_run and not error_msgs:
                    _LOGGER.debug("Attempting to create the replication user"
                                  " account.")
                    # Create the account in the server
                    rep_user_obj.create(disable_binlog=True, ssl=require_ssl)
                    # Create the localhost account in the server
                    rep_user_loc.create(disable_binlog=True, ssl=require_ssl)

                    # Give replication slave privileges
                    _grant_slave_replication(server, recovery_user,
                                             replication_host)
                    _grant_slave_replication(server, recovery_user,
                                             "localhost")

                    test_result = "PASS"

            # Not have privileges to create the replication user
            else:
                err_msg = ("The user '{0}' does not have the required "
                           "privileges to create the replication user. "
                           "It requires of CREATE USER, REPLICATION SLAVE and"
                           " SUPER privileges.".format(server.user))
                _LOGGER.error(err_msg)
                # append error message
                error_msgs.append(err_msg)

        # The rpl user exist and has REPLICATION SLAVE
        elif "REPLICATION SLAVE" not in user_priv_res[replication_user]:
            _LOGGER.debug("Replication user has REPLICATION SLAVE.")
            test_result = "PASS"

        # the rpl user exist but does not have the required privileges
        # Check if the server.user can change them
        elif "REPLICATION SLAVE" not in user_priv_res[server_user] and \
             "SUPER" not in user_priv_res[server_user]:
            _LOGGER.debug("The user %s can grant grant REPLICATION "
                          "SLAVE privilege to user %s.",
                          server.user, recovery_user)

            # Grant REPLICATION SLAVE privilege
            if not dry_run and not error_msgs:
                _LOGGER.debug("Attempting to grant REPLICATION SLAVE "
                              "privilege to the replication user %s.",
                              recovery_user)
                _grant_slave_replication(server, recovery_user,
                                         replication_host)
                test_result = "PASS"

        # server.user can not grant
        else:
            # cannot grant REPLICATION SLAVE privilege
            if "REPLICATION SLAVE" in user_priv_res[server_user]:
                err_msg = ("The given replication user {0} does not "
                           "have the REPLICATION SLAVE privilege, and "
                           "can not be granted by the user {1}, please"
                           " grant the REPLICATION SLAVE privilege."
                           "".format(recovery_user, server.user))
                error_msgs.append(err_msg)
            if "SUPER" in user_priv_res[server_user]:
                err_msg = ("The user {0} does not have SUPER "
                           "privilege required to disable the binlog."
                           "".format(server.user))
                error_msgs.append(err_msg)

        # Dictionary should not be used as argument for logger, since it
        # duplicates backslash. Convert to string and convert \\ back to \.
        dic_msg = str(user_priv_res)
        dic_msg = dic_msg.replace("\\\\", "\\")
        _LOGGER.debug("check user privileges result: %s.", dic_msg)

        # Super is required for run the Change Master command
        if "SUPER" in user_priv_res[server_user]:
            err_msg = ("The user '{0}' does not have the SUPER privilege "
                       "needed to run the CHANGE MASTER command."
                       "".format(server.user))
            error_msgs.append(err_msg)
            test_result = "FAIL"

    _LOGGER.info("* Checking user privileges... %s", test_result)


def check_server_id(req_checker, error_msgs=None):
    """Checks that the server has a unique id among the server of the group.

    :param req_checker: RequirementChecker instance used to run the tests.
    :type req_checker:  RequirementChecker
    :param error_msgs:  List of errors to which errors that occur during check
                        are appended.
    :type error_msgs:   list
    """
    if error_msgs is None:
        error_msgs = []
    server = req_checker.server
    req_dict = req_checker.req_dict

    server_id_res = req_checker.check_unique_id(req_dict[SERVER_ID])
    test_result = "PASS" if server_id_res["pass"] else "FAIL"
    _LOGGER.info("* Checking that server_id is unique... %s", test_result)
    _print_check_unique_id_results(server_id_res)
    if not server_id_res["pass"]:
        duplicate = server_id_res.get("duplicate", None)
        if duplicate is None:
            msg = ("The server_id {0} on {1} is not valid, it must be a "
                   "positive integer in the range from 1 to 4294967295."
                   "".format(server.select_variable("server_id"),
                             server))
        else:
            msg = ("The server_id {0} is already used by peer {1}"
                   "".format(server.select_variable("server_id"),
                             duplicate))
        error_msgs.append(msg)
        msg = ("The server_id must be different from the ones in use by "
               "the members of the GR group.")
        error_msgs.append(msg)


def check_server_version(req_checker, error_msgs=None):
    """Checks the server version required for GR plugin.

    :param req_checker: RequirementChecker instance used to run the tests.
    :type req_checker:  RequirementChecker
    :param error_msgs:  List of errors to which errors that occur during check
                        are appended.
    :type error_msgs:   list
    """
    if error_msgs is None:
        error_msgs = []
    req_dict = req_checker.req_dict

    server_ver_res = req_checker.check_server_version(req_dict[SERVER_VERSION])
    test_result = "PASS" if server_ver_res["pass"] else "FAIL"
    _LOGGER.info("* Checking server version... %s", test_result)
    _print_check_server_version_results(server_ver_res)
    # if pass is not True, the version is not the required
    if not server_ver_res["pass"]:
        server_ver = server_ver_res[SERVER_VERSION]
        msg = ("The version of the server is {0} and does not meet the "
               "required MySQL server version {1}"
               "".format(server_ver, REQ_GR_PLUGIN_VER))
        # append error message
        error_msgs.append(msg)


def check_options_file(req_checker, error_msgs=None, update=True,
                       verbose=False, skip_backup=False, extra_vars=None):
    """Checks the variables in the options file that requires specific values
    for GR plugin.

    :param req_checker: RequirementChecker instance used to run the tests.
    :type req_checker:  RequirementChecker
    :param error_msgs:  List of errors to which errors that occur during check
                        are appended.
    :type error_msgs:   list
    :param update:     If the options file should be updated, by default True.
    :type update:      bool.
    :param verbose:    If the output should be verbose.
    :type verbose:     bool.
    :param skip_backup: if True, skip the creation of a backup file when
                        modifying the options file.
    :type skip_backup: bool
    :param extra_vars: Dictionary with additional variables to check in the
                       configuration file.
    :type extra_vars: dict
    :return: True if the update file fulfills the requirements otherwise
             False.
    :rtype: bool
    """
    if error_msgs is None:
        error_msgs = []
    req_dict = req_checker.req_dict

    # Set variable to check in the configuration file.
    config_setting = req_dict[CONFIG_SETTINGS]
    if extra_vars:
        config_setting.update(extra_vars)

    # Check configuration file settings.
    server_var_res = req_checker.check_config_settings(
        config_setting, req_dict[OPTION_PARSER])

    test_result = "PASS" if server_var_res["pass"] else "FAIL"
    _LOGGER.info("* Checking server options from file... "
                 "%s", test_result)

    if server_var_res["pass"]:
        _LOGGER.info("Configuration file is compliant with the requirements.")
    else:
        _LOGGER.info("Some options need to be updated to enable Group "
                     "Replication.")

    missing = {}
    if "missing" in server_var_res.keys():
        missing = server_var_res.pop("missing")

    options_result = server_var_res.copy()

    if not server_var_res["pass"]:
        server_var_res.pop("pass")
        if update:
            # Attempt to update options file.
            # check we can update the file
            option_parser = req_dict[OPTION_PARSER]
            if update_option_file(option_parser, missing,
                                  server_var_res, skip_backup=skip_backup):
                results_table = _format_table_results(server_var_res, verbose)

                _LOGGER.info("Updated Options:\n%s",
                             "\n".join(results_table))
                _LOGGER.info("The options file: %s has been updated.",
                             option_parser.filename)
                # Return True, file updated with required changes.
                return True, options_result
            else:
                _LOGGER.error("Unable to update options file: %s",
                              option_parser.filename)
                # append error message
                error_msgs.append(_ERROR_UPDATE_AND_RESTART)
                error_msgs.append("Unable to update options file: {0}".format(
                    option_parser.filename))
                # Return False, could not update file.
                return False, options_result
        else:
            msg = ("Some of the configuration values on your options file "
                   "need to be changed prior to setting up Group Replication"
                   ".\nPlease update your options file and try again.")
            # append error message
            error_msgs.append(msg)
            # Return False, file requires changes.
            return False, options_result
    else:
        # The options files did not require any change
        return True, options_result


def check_server_variables(req_checker, error_msgs=None, update=True,
                           dynamic_vars=DYNAMIC_SERVER_VARS,
                           var_change_warning=False):
    """Checks the server variables that requires specific values for GR plugin

    :param req_checker: RequirementChecker instance used to run the tests.
    :type req_checker:  RequirementChecker
    :param error_msgs:  List of errors to which errors that occur during check
                        are appended.
    :type error_msgs:   list
    :param update:      If the options file should be updated, by default True
    :type update:       bool.
    :param dynamic_vars: A set of the names of the variables to attempt to
                         update if the current value is not the required.
                         By default DYNAMIC_SERVER_VARS is used.
    :type dynamic_vars: set
    :param var_change_warning: Indicate if a warning is issued when a dynamic
                               server variable is changed. By default False,
                               no warning is issued.
    :type var_change_warning: bool

    """
    if error_msgs is None:
        error_msgs = []
    # Safeguard: set dynamic_vars to an empty set in case it is None.
    if dynamic_vars is None:
        dynamic_vars = set()

    # retrieve the requirement dictionary
    req_dict = req_checker.req_dict
    server = req_checker.server

    server_var_res = req_checker.check_variable_values(
        req_dict[SERVER_VARIABLES])
    test_result = "PASS" if server_var_res["pass"] else "FAIL"
    _LOGGER.info("* Comparing options compatibility with Group "
                 "Replication... %s", test_result)
    if server_var_res["pass"]:
        _LOGGER.info("Server configuration is compliant with the "
                     "requirements.")
    else:
        restart_msg = _ERROR_RESTART_SERVER.format(req_checker.server)
        msg = _ERROR_SERVER_VARIABLES.format(server, restart_msg)
        if update:
            var_res = server_var_res
            for var_name in list(var_res.keys()):
                if var_name == "pass":
                    var_res.pop(var_name)
                    continue
                # Change the value
                in_compliance, needs_value, has = var_res.get(var_name)

                # Check if the variable is dynamic
                if in_compliance:
                    _LOGGER.debug("Variable: %s has required value: %s",
                                  var_name, has)
                    var_res.pop(var_name)
                elif var_name in dynamic_vars:
                    try:
                        _LOGGER.debug("Updating variable: %s to value: %s",
                                      var_name, needs_value)
                        # The <no value> must be substituted for an empty
                        # string ("") to set the variable with no value.
                        if needs_value == '<no value>':
                            needs_value = ""
                        server.set_variable(var_name,
                                            "'{0}'".format(needs_value),
                                            var_type='global')
                        # Issue a warning if requested.
                        if var_change_warning:
                            _LOGGER.warning("Server variable %s was changed "
                                            "from '%s' to '%s'.", var_name,
                                            has, needs_value)
                        var_res.pop(var_name)
                    except GadgetDBError as err:
                        _LOGGER.warning("Error: %s was raised updating"
                                        " %s to %s from %s", err,
                                        var_name, needs_value, has)

                else:
                    _LOGGER.debug("Variable %s is not dynamic and has value: "
                                  "%s instead of required %s.", var_name, has,
                                  needs_value)

            # Dictionary should not be used as argument for logger, since it
            # duplicates backslash. Convert to string and convert \\ back to \.
            dic_msg = str(var_res)
            dic_msg = dic_msg.replace("\\\\", "\\")
            _LOGGER.debug("The following variables are not compliant with "
                          "GR requirements: '%s'.", dic_msg)

            if var_res:
                _LOGGER.info("Incompatible server configuration was found.")
                # append error message
                error_msgs.append(msg)
            else:
                _LOGGER.info("Incompatible server configuration has been "
                             "updated.")
        else:
            _LOGGER.info("Incompatible server configuration was found.")
            # append error message
            error_msgs.append(msg)

    return server_var_res


def check_mts(req_checker, error_msgs=None, update=True,
              var_change_warning=False):
    """Check Multi-Threaded Slave settings for Group Replications.

    Check the compatibility of the Multi-Threaded Slave (MTS) settings to
    use with group replication.

    :param req_checker: Requirement checker instance used to perform the check.
    :type req_checker:  RequirementChecker
    :param error_msgs:  List of errors that occurred during the check.
    :type error_msgs:   list
    :param update:      Specify if the options file should be updated or not.
                        By default: True (option file is updated).
    :type update:       bool
    :param var_change_warning: Indicate if a warning is issued when a dynamic
                               server variable is changed. By default False,
                               no warning is issued.
    :type var_change_warning:  bool

    :return: Dictionary with the result of the check ('pass' key with the
             value 'PASS' or 'FAIL') and the variables that need to be changed
             as key (with a tuple has value that contains the required value
             for that variable and its corresponding current value).
    :rtype: dict
    """
    if error_msgs is None:
        error_msgs = []
    res = req_checker.check_mts_compatibility()

    test_result = "PASS" if res['pass'] else "FAIL"
    _LOGGER.info(
        "* Checking compatibility of Multi-Threaded Slave settings... %s",
        test_result)
    if res['pass']:
        _LOGGER.info("Multi-Threaded Slave settings are compatible with Group "
                     "Replication.")
    else:
        server = req_checker.server
        # Note: Error message must be the same as for server variables check.
        restart_msg = _ERROR_RESTART_SERVER.format(server)
        msg = _ERROR_SERVER_VARIABLES.format(server, restart_msg)
        if update:
            update_error = False
            for var_name in list(res.keys()):
                if var_name == "pass":
                    continue
                _, needs_value, has = res.get(var_name)
                try:
                    _LOGGER.debug("Updating variable: %s to value: %s",
                                  var_name, needs_value)
                    server.set_variable(var_name,
                                        "'{0}'".format(needs_value),
                                        var_type='global')
                    # Issue a warning if requested.
                    if var_change_warning:
                        _LOGGER.warning("Server variable %s was changed "
                                        "from '%s' to '%s'.", var_name,
                                        has, needs_value)
                except GadgetDBError as err:
                    update_error = True
                    _LOGGER.warning("Error updating %s to %s from %s: %s",
                                    var_name, needs_value, has, err)

            # Log different message if errors occurred updating variables.
            if update_error:
                _LOGGER.info(
                    "Incompatible Multi-Threaded Slave configuration was"
                    " found.")
                # append error message
                error_msgs.append(msg)
            else:
                _LOGGER.info("Incompatible Multi-Threaded Slave configuration "
                             "has been updated.")
        else:
            _LOGGER.info("Incompatible Multi-Threaded Slave configuration was "
                         "found.")
            # Append error message
            error_msgs.append(msg)
    return res


def check_peer_ssl_compatibility(peer_server, ssl_mode):
    """Run ssl compatibility checks for join operation.

    Knowing the ssl_mode we want to enable on the instance, this method runs
    SSL compatibility checks against the peer server to check if there is any
    SSL misconfiguration/incompatibility that prevents the instance to be added
    to the group of the peer server.

    :param peer_server:   Server that already belongs to a group.
    :type peer_server:    Server instance.
    :param ssl_mode:      GR SSL mode we want to enable on the MySQL Instance.
                          It is used to check for compatibility issues with
                          the peer server since we want to add the instance to
                          the group where the peer-server already belongs to.
    :type ssl_mode:     str
    :raise GadgetError: If there is any misconfiguration or incompatibility
                        that prevents adding the the instance to the group of
                        the peer server.
    :raise ValueError: If an unexpected ssl_mode value is provided.
    """
    # Checking SSL compatibility:
    # Get GR ssl modes from peer
    peer_gr_recovery_ssl = None
    peer_gr_ssl_mode = None

    try:
        peer_gr_recovery_ssl = peer_server.select_variable(GR_RECOVERY_USE_SSL)
    except GadgetQueryError:
        # variable does not exist, GR SSL is not enabled
        pass
    try:
        peer_gr_ssl_mode = peer_server.select_variable(GR_SSL_MODE)
    except GadgetQueryError:
        # variable does not exist, GR SSL is not enabled
        pass

    # group_replication_ssl_mode values other than "DISABLED" or "REQUIRED"
    # are not supported.
    if peer_gr_ssl_mode and peer_gr_ssl_mode.upper() not in \
            (GR_SSL_DISABLED, GR_SSL_REQUIRED):
        raise GadgetError("Cluster member {0} group_replication_ssl_mode {1} "
                          "is not supported. The supported modes are either "
                          "{2} and {3}".format(str(peer_server),
                                               peer_gr_ssl_mode,
                                               GR_SSL_DISABLED,
                                               GR_SSL_REQUIRED))

    # Transform peer_gr_recovery_ssl and peer_gr_ssl_modes into booleans
    # to ease comparisons between their values.
    bool_peer_gr_recovery_ssl = peer_gr_recovery_ssl and \
        peer_gr_recovery_ssl.upper() in ("1", "ON")
    bool_peer_gr_ssl_mode = peer_gr_ssl_mode and \
        peer_gr_ssl_mode.upper() == GR_SSL_REQUIRED

    # Both group_replication_ssl_mode and group_replication_recovery_ssl need
    # to be enabled or disabled on the peer server, otherwise it is not
    # supported.
    if bool_peer_gr_recovery_ssl != bool_peer_gr_ssl_mode:
        raise GadgetError("Cluster member {0} configuration for "
                          "group_replication_ssl_mode and "
                          "group_replication_recovery_use_ssl is not "
                          "supported. If group_replication_ssl_mode is "
                          "{1} then group_replication_recovery_ssl "
                          "needs to be ON. Otherwise if "
                          "group_replication_ssl_mode is {2} then "
                          "group_replication_recovery_ssl needs to be OFF."
                          "".format(str(peer_server), GR_SSL_REQUIRED,
                                    GR_SSL_DISABLED))

    # Raise an error if GR SSL is enabled on the peer server but
    # ssl-mode=DISABLED was used. Use shell option name on the error message.
    if bool_peer_gr_ssl_mode and ssl_mode == GR_SSL_DISABLED:
        raise GadgetError("Cannot use memberSslMode:'{0}' because "
                          "Group Replication SSL support is enabled on the "
                          "Cluster. Use memberSslMode:'AUTO' or "
                          "memberSslMode:'{1}'.".format(GR_SSL_DISABLED,
                                                        GR_SSL_REQUIRED))

    # Raise and error if GR SSL is disabled on the peer-server but
    # ssl-mode=REQUIRED was used. Use shell option name on the error message.
    if not bool_peer_gr_ssl_mode and ssl_mode == GR_SSL_REQUIRED:
        raise GadgetError("Cannot use memberSslMode:'{0}' because "
                          "Group Replication SSL support is disabled on "
                          "the Cluster. Use memberSslMode:'AUTO' or "
                          "memberSslMode:'{1}'.".format(GR_SSL_REQUIRED,
                                                        GR_SSL_DISABLED))


def check_server_requirements(server, req_dict, rpl_settings, verbose=False,
                              dry_run=False, skip_schema_checks=False,
                              update=True, skip_backup=False,
                              dynamic_vars=DYNAMIC_SERVER_VARS,
                              var_change_warning=False):
    """Runs the checking of the group replication requirements

    :param server:        Server to check GR requirements.
    :type server:         Server instance.
    :param req_dict:      Dictionary with the requirements to check.
    :type req_dict:       dict.
    :param rpl_settings:  Replication settings; replication user, user pw,
                          recovery user, ssl_mode.
    :type rpl_settings:   dict.
    :param verbose:       If the output should be verbose.
    :type verbose:        bool.
    :param dry_run:       If true no actions will affect the given server.
    :type dry_run:        bool.
    :param skip_schema_checks: If the check for engine Innodb and primary key
                          should be skip, by default False.
    :type skip_schema_checks: bool.
    :param update:        If the options file should be updated, by default
                          True.
    :type update:         bool.
    :param skip_backup: if True, skip the creation of a backup file when
                        modifying the options file.
    :type skip_backup: bool
    :param dynamic_vars: A set of the names of the variables to attempt to
                         update if the current value is not the required.
                         By default DYNAMIC_SERVER_VARS is used.
    :type dynamic_vars: set
    :param var_change_warning: Indicate if a warning is issued when a dynamic
                               server variable is changed. By default False,
                               no warning is issued.
    :type var_change_warning: bool

    :raise GadgetError: If the server does not meet the requirements or
                        Gadget could not make the required changes to
                        fulfill them.
    """
    # The requirement checker instance will be used to run the tests.
    req_checker = RequirementChecker(req_dict, server)

    # Check if the requirements were met
    error_msgs = {}
    options_res = None

    # Checking server configuration variables
    if SERVER_VARIABLES in req_dict.keys():
        errors = []
        options_res = check_server_variables(req_checker, errors, update,
                                             dynamic_vars, var_change_warning)
        if errors:
            error_msgs[SERVER_VARIABLES] = errors

    # Checking server configuration variables that must match on peer server
    if PEER_SERVER_VARIABLES in req_dict.keys():
        errors = []
        check_peer_variables(req_checker, errors, verbose)
        if errors:
            error_msgs[PEER_SERVER_VARIABLES] = errors

    # Checking server version
    if SERVER_VERSION in req_dict.keys():
        errors = []
        check_server_version(req_checker, errors)
        if errors:
            error_msgs[SERVER_VERSION] = errors

    # Checking server id
    if SERVER_ID in req_dict.keys():
        errors = []
        check_server_id(req_checker, errors)
        # Error if the server_id is not valid (value zero or duplicate found).
        # Note: A restart is required despite the server_id being a dynamic
        # variable, otherwise an error is issued by the CHANGE MASTER
        # statement when trying to join the instance to a group.
        if errors:
            # Update the option file if provide and update is requested.
            if update and OPTION_PARSER in req_dict.keys():
                # Generate a new random server_id.
                server_id = generate_server_id()
                # Set the new server_id to be changed for the option file.
                req_dict[CONFIG_SETTINGS]["server_id"] = {
                    ONE_OF: (server_id,)}
                restart_msg = _ERROR_RESTART_SERVER.format(req_checker.server)
                id_msg = ("A new server_id with value {0} was set in the "
                          "options file.\n{1}").format(server_id, restart_msg)
                errors.append(id_msg)
                error_msgs[SERVER_ID] = errors
            else:
                error_msgs[SERVER_ID] = errors
        _LOGGER.info("")

    # Checking MTS settings
    mts_opt_file = {}
    if MTS_SETTINGS in req_dict.keys():
        errors = []
        mts_opt_res = check_mts(req_checker, errors, update,
                                var_change_warning)
        if 'pass' in mts_opt_res and not mts_opt_res['pass']:
            # Update the result for SERVER_VARIABLES to include the MTS
            # settings that need to be changed.
            options_res.update(mts_opt_res)
            # Create options to check in config file if needed.
            if 'slave_parallel_type' in mts_opt_res:
                mts_opt_file['slave_parallel_type'] = \
                    {ONE_OF: ("LOGICAL_CLOCK",)}
            if 'slave_preserve_commit_order' in mts_opt_res:
                mts_opt_file['slave_preserve_commit_order'] = \
                    {ONE_OF: ("ON", "1")}
        if errors:
            # Set SERVER_VARIABLES errors if not already set.
            # Note: Errors for MTS_SETTINGS and SERVER_VARIABLES are the
            # same to maintain the output expected by the AdminAPI.
            if SERVER_VARIABLES not in error_msgs:
                error_msgs[SERVER_VARIABLES] = errors

    # Checking server options from file
    options_results = None
    if OPTION_PARSER in req_dict.keys():
        errors = []
        res = check_options_file(req_checker, errors, update, verbose,
                                 skip_backup=skip_backup,
                                 extra_vars=mts_opt_file)
        options_results = res[1]
        if options_res is not None:
            if "pass" in options_res.keys():
                options_res.pop("pass")
            for var_name in options_res:
                if var_name not in options_results.keys():
                    continue
                _, _, cur_val = options_res[var_name]
                # Get equivalent value set. If value is not one of the
                # ON, OFF, 0 or 1, then by default it will return a set with
                # the cur_val.
                eq_val_set = EQUIVALENT_OPTION_VALUES.get(
                    options_results[var_name][2], {cur_val, })
                if var_name in options_results.keys() and \
                   cur_val != "<no value>" and cur_val not in eq_val_set:
                    _LOGGER.warning("The value %s of the option %s in the "
                                    "server %s differs from the value %s in "
                                    "in the given defaults file.",
                                    cur_val, var_name, server,
                                    options_results[var_name][2])

        if errors:
            error_msgs[CONFIG_SETTINGS] = errors

    # Line separator of test.
    _LOGGER.info("")

    # Check the replication user exist or can be created.
    if USER_PRIVILEGES in req_dict.keys():
        errors = []
        check_user_privileges(req_checker, rpl_settings, dry_run, verbose,
                              errors)
        if errors:
            error_msgs[USER_PRIVILEGES] = errors

    # Check if the server has Tables and if has if these complains the
    # requirements from GR plugin (Engine InnoDB and has Primary key).
    if not skip_schema_checks:
        errors = []
        check_compliance(req_checker, errors)
        if errors:
            error_msgs["schema_check"] = errors

    # Line separator of test and results.
    _LOGGER.info("")

    # If no errors found, setup can proceed.
    if not error_msgs:
        return True
    else:
        error_msg_list = []
        for test_name in error_msgs:
            error_msg_list.extend(error_msgs[test_name])
            # Attach comparison results table
            if test_name == SERVER_VARIABLES:
                results_table = _format_table_results(options_res, verbose)
                error_msg_list.extend(results_table)
            # Whit update option the table is already logged.
            elif test_name == CONFIG_SETTINGS and not update:
                results_table = _format_table_results(options_results,
                                                      verbose)
                error_msg_list.extend(results_table)

        # Raise an error since the server does not met the requirements for GR
        err_msg = ("The operation could not continue due to the "
                   "following requirements not being met:\n{0}"
                   "".format("\n".join(error_msg_list)))

        raise GadgetError(err_msg)


def get_gr_members(server, exclude_server=None):
    """Obtains the list of all the members of the Group Replication group.

    This method attempts to connect to the remaining membership of the same
    group that the given server is member, using the same credentials, except
    the one specified as server to exclude.

    :param server:  Server instance to query information of all the members of
                    that belongs to the same Group Replication group.
    :type server:   Server
    :param exclude_server: Server to exclude from the list of returned peer
                           servers.
     :type exclude_server:  Server

    :return: List of server instances that belongs to the same group and it
             shares the same credentials.
    :rtype: list
    """
    members = []
    peers_info = server.exec_query(
        "select MEMBER_HOST, MEMBER_PORT from {0}"
        "".format(REP_GROUP_MEMBERS_TABLE))
    if exclude_server:
        exclude_hosts = [exclude_server.host]
        # Include FQDN as an alias to match the host of the excluded server.
        if exclude_server.host in ("localhost", "127.0.0.1", "::1"):
            exclude_hosts.append(socket.getfqdn())
    if peers_info:
        for peer_info in peers_info:
            if (exclude_server and peer_info[0] in exclude_hosts and
                    str(exclude_server.port) == peer_info[1]):
                # Skip peer if it matches the server to exclude.
                continue
            conn_val = server.get_connection_values()
            conn_val["host"] = peer_info[0]
            conn_val["port"] = peer_info[1]
            member = None
            try:
                member = get_server(server_info=conn_val)
                members.append(member)
            except GadgetError as err:
                _LOGGER.debug(err.errmsg)
                if member is not None:
                    _LOGGER.warning("Unable to verify server_id of %s",
                                    member)
                else:
                    _LOGGER.warning("Unable to verify server_id of %s:%s",
                                    conn_val["host"], conn_val["port"])

    return members


def get_req_dict(server, replication_user, peer_server=None, option_file=None):
    """Create a dictionary with the requirements for setting GR plugin.

    :param server: The server in which the command wil be perform..
    :type server:  Server instance (mysql_gadgets.common.server).
    :param replication_user: Replication user in the form <user_name>@<host>.
    :type replication_user:  String
    :param peer_server: A server member of the GR group from where obtain the
                        information of the group.
    :type peer_server:  Server instance (mysql_gadgets.common.server).
    :param option_file: The system path of the option file used to start
                        the server.
    :type option_file:  string

    :return: A dictionary with the options and required values to check on the
             server.
    :rtype: dict
    """
    req_dict = DEFAULT_REQ_DICT.copy()
    req_dict[SERVER_VARIABLES] = GR_REQUIRED_CONFIG
    # Add the report_port variable, the value is dynamically taken from
    # current port, this avoids communication errors retrieving peer_addresses
    port_str = str(server.port)
    req_dict[SERVER_VARIABLES]['report_port'] = {ONE_OF: (port_str,)}

    if option_file is not None:
        # Add the option parser to be used to check and update option file
        req_dict[OPTION_PARSER] = MySQLOptionsParser(option_file)
        req_dict[CONFIG_SETTINGS] = GR_REQUIRED_OPTIONS
        # Add the report_port
        req_dict[CONFIG_SETTINGS]['report_port'] = {ONE_OF: (port_str,)}

    super_user = "{0}@'{1}'".format(server.user, server.host)

    if replication_user is not None:
        req_dict[USER_PRIVILEGES] = {
            replication_user: {"REPLICATION SLAVE"},
            super_user: {"CREATE USER", "SUPER", "REPLICATION SLAVE"},
        }

    # add the peer list used to verify server_id uniqueness
    if peer_server is not None:
        req_dict[SERVER_ID] = {"peers": get_gr_members(peer_server, server)}
        req_dict[PEER_SERVER_VARIABLES] = {
            "peer": peer_server,
            "peer_variables": PEER_VARIABLES,
        }
    else:
        req_dict[SERVER_ID] = {"peers": []}

    # Add MTS setting check
    req_dict[MTS_SETTINGS] = {}

    return req_dict


def get_req_dict_user_check(server, replication_user):
    """Create a dictionary with the requirements for check an user.

    :param server: The server in which the command wil be perform.
    :type server:  Server instance (mysql_gadgets.common.server)
    :param replication_user: Replication user in the form <user_name>@<host>.
    :type replication_user:  String

    :return: A dictionary with the options and required values to check on the
             server.
    :rtype: dict
    """
    req_dict = {}
    # The requirement checker instance will be used to run the tests.
    super_user = "{0}@'{1}'".format(server.user, server.host)
    req_dict[USER_PRIVILEGES] = {
        replication_user: {"REPLICATION SLAVE"},
        super_user: {"CREATE USER", "REPLICATION SLAVE"},
    }
    return req_dict


def get_req_dict_for_opt_file(option_file):
    """Create a dictionary with the requirements for check an option file.

    :param option_file: The system path of the option file used to start
                        the server.
    :type option_file:  string

    :return: A dictionary whit the options and required values to check on the
             server.
    :rtype: dict
    """
    req_dict = {}
    # Add the option parser to be used to check and update option file
    req_dict[OPTION_PARSER] = MySQLOptionsParser(option_file)
    req_dict[CONFIG_SETTINGS] = GR_REQUIRED_OPTIONS

    return req_dict


def get_rpl_usr(options=None):
    """Initializer of the class server.

    :param options:  A dictionary with the following options
        replication_user:   The user name and host in the form user@host to
                            use to connect between the servers in the
                            group. Optional if not given: 'rpl_user@%'
        rep_user_passwd:    The password for the replication user.

    :raise GadgetError: If replication user password is not provided.
    """
    rep_user_passwd = options.get("rep_user_passwd", None)

    replication_user = options.get("replication_user", None)
    # If replication user is not given
    if replication_user is None:
        replication_user = "{0}@'{1}'".format("rpl_user", '%')
        recovery_user = "rpl_user"
        host = '%'
        _LOGGER.debug("No replication_user has been given, using: %s",
                      replication_user)
    else:
        if "@" not in replication_user:
            replication_user = "{0}@'%'".format(replication_user)
        user, host = parse_user_host(replication_user)
        replication_user = "{0}@'{1}'".format(user, host)
        recovery_user = user
        _LOGGER.debug("Using replication_user: %s",
                      replication_user)

    # Resolve the hostname to allow remote configurations.
    if host in ["localhost", "127.0.0.1", "::1"]:
        host = socket.getfqdn()

    if rep_user_passwd is None or rep_user_passwd == "":
        raise GadgetError("A password for the replication user '{0}' was not"
                          " given.".format(recovery_user))

    rpl_user_dict = {
        "replication_user": replication_user,
        "recovery_user": recovery_user,
        "rep_user_passwd": rep_user_passwd,
        "host": host,
        "ssl_mode": options.get("ssl_mode", GR_SSL_REQUIRED)
    }

    rpl_user_dict_hidden_pw = rpl_user_dict.copy()
    rpl_user_dict_hidden_pw["rep_user_passwd"] = "******"
    # Dictionary should not be used as argument for logger, since it
    # duplicates backslash. Convert to string and convert \\ back to \.
    dic_msg = str(rpl_user_dict_hidden_pw)
    dic_msg = dic_msg.replace("\\\\", "\\")
    _LOGGER.debug("->rpl_user_dict %s", dic_msg)
    return rpl_user_dict


def get_gr_config_vars(local_address, options=None, options_parser=None,
                       peer_local_address=None,
                       defaults_options=DEFAULTS_FILE_OPTIONS,
                       server_id=None):
    """config_vars

    :param local_address:   the host:port that member will expose itself to
                            be contacted by the group.
    :type local_address:    string
    :param options:  A dictionary with the following options
        group_name          Group Replication name in the form of UUID,
                            optional only for the bootstrap command.
        single_primary:     Indicates the gr_single_primary_mode; ON or OFF,
                            by default ON.
        group_seeds:        The list of peer address (servers part of the
                            group). A comma separated list with the format:
                            <host>:<port>[,<host>:<port>].
        ip_whitelist:       The list of hosts allowed to connect. A comma
                            separated list of IP addresses or CIDR notation,
                            for example: 192.168.1.0/24,10.0.0.1.
                            By default: AUTOMATIC (private network addresses
                            automatically allowed).
    :param options_parser: Option file parser used to read the values in the
                           options file if available.
    :type options_parser: MySQLOptionsParser
    :param peer_local_address: The host:port that the peer member uses to
                               expose itself to be contacted by the group.
    :type peer_local_address: string
    :param defaults_options: the options to read from the defaults file.
                             By default DEFAULTS_FILE_OPTIONS_LIST is used.
    :type defaults_options: set
    :param server_id: server_id of the server

    :return: config_vars
    :rtype: dict
    """

    if defaults_options is None:
        defaults_options = ()

    pickable_options = list(defaults_options)

    if options is None:
        options = {}

    # Setup GR_CONFIG_VARIABLES to be set later
    gr_config_vars = {
        GR_GROUP_NAME: options.get("group_name", None),
        GR_LOCAL_ADDRESS: local_address,
        GR_SINGLE_PRIMARY_MODE: options.get("single_primary", None),
        GR_GROUP_SEEDS: options.get("group_seeds", None),
        GR_IP_WHITELIST: options.get("ip_whitelist", None)
    }

    # Use the value passed as an argument and not the value from
    # the option file.
    # Remove the options whose value is not None to avoid overwrite them
    for option_name in pickable_options:
        if option_name in gr_config_vars and \
           gr_config_vars[option_name] is not None:
            pickable_options.remove(option_name)

    # If not ssl_mode, add the SSL configuration
    # By default ssl_mode is REQUIRED
    ssl_mode = options.get("ssl_mode", GR_SSL_REQUIRED)
    if ssl_mode == GR_SSL_REQUIRED:
        ssl_config = {
            GR_RECOVERY_USE_SSL: "ON",
            GR_SSL_MODE: GR_SSL_REQUIRED,
        }
        if options.get("ssl_ca", None) is not None:
            ssl_config[GR_RECOVERY_SSL_CA] = options.get("ssl_ca")
        if options.get("ssl_cert", None) is not None:
            ssl_config[GR_RECOVERY_SSL_CERT] = options.get("ssl_cert")
        if options.get("ssl_key", None) is not None:
            ssl_config[GR_RECOVERY_SSL_KEY] = options.get("ssl_key")
    elif ssl_mode == GR_SSL_DISABLED:
        ssl_config = {
            GR_RECOVERY_USE_SSL: "OFF",
            GR_SSL_MODE: GR_SSL_DISABLED,
        }
    gr_config_vars.update(ssl_config)

    # Set GR group seeds with the value from the peer local_address.
    if gr_config_vars[GR_GROUP_SEEDS] is None:
        gr_config_vars[GR_GROUP_SEEDS] = peer_local_address

    # Pick auto_increment params depending on single_primary mode
    if options.get("single_primary", None) == "ON":
        # GR plugin will override auto_increment params to values that are
        # suitable for multi-primary, even when in single-primary, except
        # in 8.0; so we must override it back.
        # offset of 2 is picked because if it's 1, the GR plugin will think
        # is the default value and will override it
        gr_config_vars[AUTO_INCREMENT_INCREMENT] = 1
        gr_config_vars[AUTO_INCREMENT_OFFSET] = 2
    else:
        # If in multi-primary mode, we need to ensure that the offset value
        # is smaller than auto_increment_increment (which defaults to 7)
        if server_id:
            gr_config_vars[AUTO_INCREMENT_OFFSET] = 1 + int(server_id) % 7
            gr_config_vars[AUTO_INCREMENT_INCREMENT] = 7

    # Read values from option file if available.
    if options_parser is not None:
        for option_name in pickable_options:
            option_value = None
            # Retrieve the option from the defaults file.
            if options_parser.has_option('mysqld',
                                         LOOSE_PREFIX.format(option_name)):
                # the option with the loose prefix exists
                option_value = options_parser.get(
                    'mysqld', LOOSE_PREFIX.format(option_name))
            if options_parser.has_option('mysqld', option_name):
                # overwrite the value from the option without the prefix
                option_value = options_parser.get('mysqld', option_name)

            if option_value is not None:
                # The values given in options have precedence over the ones in
                # the defaults_file.
                if option_name in gr_config_vars:

                    # Check if option requires to append the value from the
                    # defaults file.
                    if option_name in APPEND_VALUE_OPTIONS:
                        # Only append the value if it is not already included
                        # Remove quotes.
                        if option_value[0] == option_value[-1] and \
                           option_value[0] in ["'", '"']:
                            option_value = option_value[1:-1]
                        if gr_config_vars[option_name][0] == \
                           gr_config_vars[option_name][-1] and \
                           gr_config_vars[option_name][0] in ["'", '"']:
                            cur_val = gr_config_vars[option_name][1:-1]

                        # Create a set for each list
                        cur_val_set = set(
                            e.strip() for e in cur_val.split(','))
                        option_value_set = set(
                            e.strip() for e in option_value.split(','))
                        if cur_val_set ^ option_value_set:
                            # Join the values from the union of the two sets
                            # on a single string list separated by commas
                            gr_config_vars[option_name] = ",".join(
                                option_value_set.union(cur_val_set))

                    # Set value with the one from the defaults file.
                    elif gr_config_vars[option_name] is None:
                        # The current value for the option is None
                        gr_config_vars[option_name] = option_value

                # Use value from defaults file, since option is not provided.
                else:
                    # Save the value found as no other was provided.
                    gr_config_vars[option_name] = option_value

    # Add surrounding quotes in case the option does not have it already.
    for option_name in gr_config_vars:
        if gr_config_vars[option_name] is not None and \
           type(gr_config_vars[option_name]) is str and \
           gr_config_vars[option_name][0] not in ["'", '"']:
            gr_config_vars[option_name] = "'{0}'".format(
                gr_config_vars[option_name])

    return gr_config_vars


def persist_gr_config(defaults_path, config_values=None, set_on=True,
                      dry_run=False, skip_backup=False):
    """Write provided Group Replication config values to the option file.

    This method persists the given Group Replication (GR) configuration values
    to the provided option file. The 'group_replication_start_on_boot' option
    is also set, based on the 'set_on' parameter, even if not provided in the
    configuration values to persist.

    :param defaults_path: The system path of the option file used to start
                          the server or the MySQLOptionsParser used to modify
                          the options file.
    :type defaults_path: string or MySQLOptionsParser
    :param config_values: the dictionary with the GR values to persist.
    :type config_values: dictionary
    :param set_on: Sets the value of 'group_replication_start_on_boot' to "ON"
                   if True otherwise "OFF", by default True.
    :type set_on: boolean
    :param dry_run: Indicate if dry-run mode is used or not. By default False,
                    dry-run not used and no changes made to the option file.
    :type dry_run: boolean
    :param skip_backup: if True, skip the creation of a backup file when
                        modifying the options file.
    :type skip_backup: boolean
    """
    if defaults_path is None or defaults_path == "":
        _LOGGER.warning("No path to the defaults file was given.")
    # Create a new MySQLOptionsParser instance if needed
    if isinstance(defaults_path, MySQLOptionsParser):
        options_parser = defaults_path
    else:
        options_parser = MySQLOptionsParser(defaults_path)

    if config_values is None:
        config_values = {}

    else:
        # Remove quotes
        for option_name, value in config_values.items():
            if (len(value) > 0 and value[0] == value[-1] and
                    value[0] in {'"', "'"}):
                config_values[option_name] = \
                    config_values[option_name][1:-1]

        # Configure aliases options.
        _LOGGER.debug("Configuring options to be persisted")
        for option_name in config_values:
            # Check if the prefixed "loose-" option exists already and if
            # so replace its value
            if options_parser.has_option(
                    'mysqld', LOOSE_PREFIX.format(option_name)):
                _LOGGER.debug("Option %s with loose prefix found", option_name)
                # Save the value with "loose-" prefix.
                config_values[LOOSE_PREFIX.format(option_name)] = \
                    config_values.pop(option_name)

                # else: The option without the prefix is already set.

    # Set the value for the 'group_replication_start_on_boot' option.
    if options_parser.has_section('mysqld') and \
       options_parser.has_option('mysqld',
                                 LOOSE_PREFIX.format(GR_START_ON_BOOT)):
        config_values[LOOSE_PREFIX.format(
            GR_START_ON_BOOT)] = "ON" if set_on else "OFF"

        _LOGGER.debug("Updating %s to %s",
                      LOOSE_PREFIX.format(GR_START_ON_BOOT),
                      config_values[LOOSE_PREFIX.format(GR_START_ON_BOOT)])

    else:
        config_values[GR_START_ON_BOOT] = "ON" if set_on else "OFF"

        _LOGGER.debug("Updating %s to %s", GR_START_ON_BOOT,
                      config_values[GR_START_ON_BOOT])

    # Persist the configuration value to the option file.
    if not dry_run:
        # Note: No need to separate values to update from missing values, all
        # config_values can be passed as missing value, they will be updated
        # correctly.
        update_option_file(options_parser, config_values, {},
                           skip_backup=skip_backup)


def check_gr_plugin_is_installed(server, option_file, dry_run=False):
    """Check gr plugin is installed and reinstall if it's disabled.

    :param server: The server instance to verify.
    :type server:  Server
    :param option_file: The path for configuration file of the given server.
    :type option_file:  string
    :param dry_run: If changes should not take efect, by default False.
    :type dry_run:  boolean

    :raise GadgetError: If the group replication plugin is disabled.
    """
    # Verify the plugin is installed
    if not server.is_plugin_installed(GR_PLUGIN_NAME, silence_warnings=True):
        # Can not be disabled without be installed.
        install_plugin(server, dry_run)

        if option_file:
            # Update the group_replication_start_on defaults file.
            # Note: Set group_replication_start_on_boot=OFF (set_on=False)
            #       as is not configured.
            config_values = {GR_PLUGIN_NAME: "ON"}
            persist_gr_config(option_file, config_values, set_on=False,
                              dry_run=dry_run)
    # verify the group replication is not Disabled in the server.
    if server.supports_plugin(GR_PLUGIN_NAME, "DISABLED") and not dry_run:
        if option_file:
            _LOGGER.info(_GR_PLUGIN_DISABLED_REQ_UNINSTALL, server)
            # Update the group_replication_start_on defaults file.
            # Note: Set group_replication_start_on_boot=OFF (set_on=False)
            #       as is not configured.
            config_values = {GR_PLUGIN_NAME: "ON"}
            persist_gr_config(option_file, config_values, set_on=False,
                              dry_run=dry_run)
            # uninstall the gr plugin so it can be installed again.
            uninstall_plugin(server, dry_run)
            # Reinstall the GR plugin
            install_plugin(server, dry_run)
        else:
            raise GadgetError(_ERROR_GR_PLUGIN_DISABLED.format(server))


def get_gr_configs_from_instance(server):
    """Get a dict with the values of the Group Replication variables.

    :param server: The MySQL instance to verify.
    :type server:  Server
    :return: Dictionary with the GR variable values
    :rtype: OrderedDict
    """
    gr_configs = collections.OrderedDict()
    gr_vars = server.exec_query("show variables like 'group_replication_%'")
    for variable_name, value in gr_vars:
        gr_configs[variable_name] = value
    gr_vars = server.exec_query("show variables like 'auto_increment_%'")
    for variable_name, value in gr_vars:
        gr_configs[variable_name] = value
    return gr_configs
