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
This module contains the Group Replication Admin operation methods
"""

import random
import textwrap
import time
# Use backported OrderedDict if not available (for Python 2.6)
try:
    from collections import OrderedDict
except ImportError:
    from mysql_gadgets.common.ordered_dict_backport import OrderedDict

from mysql_gadgets.exceptions import GadgetError

from mysql_gadgets.common.format import get_max_display_width
from mysql_gadgets.common import logging
from mysql_gadgets.common.logging import STEP_LOG_LEVEL_VALUE
from mysql_gadgets.common.server import get_server, LocalErrorLog
from mysql_gadgets.common.group_replication import (
    check_gr_plugin_is_installed,
    GR_GROUP_NAME,
    GR_GROUP_SEEDS,
    GR_SINGLE_PRIMARY_MODE,
    HAVE_SSL,
    GR_SSL_DISABLED,
    GR_SSL_REQUIRED,
    OPTION_PARSER,
    persist_gr_config,
    REP_CONN_STATUS_TABLE,
    REP_GROUP_MEMBERS_TABLE,
    REP_MEMBER_STATS_TABLE,
    REP_APPLIER_STATUS_BY_WORKER,
    get_gr_configs_from_instance,
    get_gr_local_address_from,
    get_gr_name_from_peer,
    setup_gr_config,
    GR_IP_WHITELIST)
from mysql_gadgets.common.connection_parser import clean_IPv6
from mysql_gadgets.common.group_replication import (
    do_change_master, check_peer_ssl_compatibility,
    check_server_requirements, get_gr_config_vars, get_gr_variable_from_peer,
    get_member_state, get_req_dict, get_req_dict_for_opt_file,
    get_req_dict_user_check, get_rpl_usr, get_group_uuid_name,
    is_active_member, is_member_of_group, set_bootstrap, start_gr_plugin,
    stop_gr_plugin, unset_bootstrap, validate_group_name)
from mysql_gadgets.common.tools import is_listening

_LOGGER = logging.getLogger(__name__)

# Operations
CHECK = "check"
HEALTH = "health"
JOIN = "join"
LEAVE = "leave"
STATUS = "status"
START = "start"

MEMBER_ID = "MEMBER_ID"
MEMBER_HOST = "MEMBER_HOST"
MEMBER_PORT = "MEMBER_PORT"
MEMBER_STATE = "MEMBER_STATE"
VIEW_ID = "VIEW_ID"
COUNT_TRANSACTIONS_IN_QUEUE = "COUNT_TRANSACTIONS_IN_QUEUE"
COUNT_TRANSACTIONS_CHECKED = "COUNT_TRANSACTIONS_CHECKED"
COUNT_CONFLICTS_DETECTED = "COUNT_CONFLICTS_DETECTED"
COUNT_TRANSACTIONS_ROWS_VALIDATING = "COUNT_TRANSACTIONS_ROWS_VALIDATING"
TRANSACTIONS_COMMITTED_ALL_MEMBERS = "TRANSACTIONS_COMMITTED_ALL_MEMBERS"
LAST_CONFLICT_FREE_TRANSACTION = "LAST_CONFLICT_FREE_TRANSACTION"

_ERROR_ALREADY_A_MEMBER = ("The server {0} is already a member of a GR group."
                           " In order to {1} a different group, the server "
                           "must first leave the current GR group")

_ERROR_NOT_A_MEMBER = ("The server {0} is not a member of a Group Replication"
                       " group.")

_ERROR_NO_HAVE_SSL = ("MySQL Instance {0} has SSL support disabled. "
                      "Use the memberSslMode:'{1}' if you want to proceed "
                      "with the configuration without SSL support."
                      .format("{0}", GR_SSL_DISABLED))

_ERROR_NO_SERVER = "No server was given."

_ERROR_UNABLE_TO_GET = "Unable to get the {0} from the peer server '{1}'."

_ERROR_INVALID_LOCAL_ADDRESS_PORT = (
    "Invalid port '{0}' for localAddress option. The port must be an integer "
    "between 1 and 65535.")
_ERROR_LOCAL_ADDRESS_PORT_IN_USE = (
    "The port '{0}' for localAddress option is already in use. Specify an "
    "available port to be used with localAddress option or free port '{0}'.")

_RUNNING_COMMAND = "Running {0} command on {1}."

_WARN_DRY_RUN_USED = ("None of the previous changes were made. Changes are "
                      "only made if the command is executed without the "
                      "--dry-run option.")

max_screen_width = get_max_display_width()
# Define how much time wait before check for super_read_only to be unset
WAIT_SECONDS = 1
# Define for how long time wait super_read_only to be unset
TIME_OUT = 15 * 60  # 15 minutes


def resolve_gr_local_address(gr_host, server_host, server_port):
    """Resolves Group Replication local address (host, port).

    If a host is not found on gr_host, the returned host is the one given on
    server_host.
    If a port is not found on gr_host, the returned port is the one given on
    server_port plus 10000, unless the result is higher than 65535, in that
    case a random port number will be generated.

    :param gr_host:     Group replication host address in the format:
                        <host>[:<port>] (i.e., host or host and port separated
                        by ':').
    :type gr_host:      string
    :param server_host: The host where the MySQL server is running.
    :type server_host:  string
    :param server_port: The port that the MySQL server is using.
    :type server_port:  string

    :raise GadgetError:  If could not found a free port.

    :return: A tuple with host and port.
    :rtype:  tuple
    """
    is_port_specified = False

    # No info provided, use the server to generate it.
    if gr_host is None or gr_host == "":
        gr_host = server_host
        local_port = str(int(server_port) + 10000)

    # gr_host can have both elements; host and port, but be aware of IPv6
    elif len(gr_host.rsplit(":", 1)) == 2 and gr_host[-1] != "]":
        gr_host, local_port = gr_host.rsplit(":", 1)
        if not gr_host:
            gr_host = server_host
        if not local_port:
            local_port = str(int(server_port) + 10000)
        elif (not local_port.isdigit()
              or int(local_port) <= 0 or int(local_port) > 65535):
            # Raise an error if the specified port part is invalid.
            raise GadgetError(
                _ERROR_INVALID_LOCAL_ADDRESS_PORT.format(local_port))
        else:
            is_port_specified = True

    # Try to get the port only
    elif gr_host.isdigit():
        local_port = gr_host
        gr_host = server_host
        is_port_specified = True
        # Raise an error if the specified port is invalid (out of range).
        if int(local_port) <= 0 or int(local_port) > 65535:
            raise GadgetError(
                _ERROR_INVALID_LOCAL_ADDRESS_PORT.format(local_port))

    # Generate a local port based on the + 10000 rule.
    else:
        local_port = str(int(server_port) + 10000)

    # in case the gr_host is a IPv6 remove the square brackets '[ ]'
    gr_host = clean_IPv6(gr_host)

    # Generate a random port if out of range.
    if int(local_port) <= 0 or int(local_port) > 65535:
        local_port = str(random.randint(10000, 65535))
        # gr_host is host address

    # verify the port is not in use.
    tries = 1
    port_found = False
    while tries < 5 and not port_found:
        tries += 1
        if not is_listening(server_host, int(local_port)):
            port_found = True
        else:
            if is_port_specified:
                raise GadgetError(
                    _ERROR_LOCAL_ADDRESS_PORT_IN_USE.format(local_port))
            else:
                local_port = str(random.randint(10000, 65535))

    if not port_found:
        raise GadgetError("Unable to find an available port on which the "
                          "member will expose itself to be contacted by the "
                          "other members of the group. Specify an available "
                          "port to be used with localAddress option or free "
                          "port {0}.".format(str(int(server_port) + 10000)))

    return gr_host, local_port


def _report_dict(result_dict, template, header=None, hder_template=None,
                 reporter=None, show_empty_values=False):
    """Log result_dict contents

    :param result_dict:   The results to report.
    :type result_dict:    dict
    :param template:      A string template to format the output.
    :type template:       string
    :param header:        The key name that is used as a header. Default, None.
    :type header:         string
    :param hder_template: String template to format the header. Default, None.
    :type hder_template:  string
    :param reporter:      The function used to report the results.
    :type reporter:       function
    :param show_empty_values: If additional information should be reported.
                          Default, False.
    :type show_empty_values: boolean
    """
    if reporter is None:
        reporter = _LOGGER.info

    for elem_name in result_dict.keys():
        # Elements with "" value or None value are hidden by default unless
        # the show_empty_values argument is set to True, in such case the name
        # of the element will be displayed alone, since his value is not
        # printable.
        value = "" if result_dict[elem_name] is None \
            else result_dict[elem_name]

        # Hide not printable values.
        if value or show_empty_values:
            lines = textwrap.wrap(value, width=(max_screen_width - 20))
            # textwrap returns empty for ""
            if len(lines) == 0:
                lines.append("")

            if header is not None and elem_name in header:
                name = elem_name.replace("_", " ").capitalize()
                output = hder_template.format("", name, value)
                reporter(output)
            else:
                name = elem_name.replace("_", " ").capitalize()
                output = (template.format("", name, lines[0]))
                reporter(output)
                for line in lines[1:]:
                    output = "{0}{1}".format(" " * (len(output) -
                                                    len(lines[0])), line)
                    reporter(output)


# Operation methods
def check(**kwargs):
    """Check and update instance configurations relative to Group Replication.

    :param kwargs:    Keyword arguments:
                        update:     Make changes to the options file.
                        verbose:    If the command should show more verbose
                                    information.
                        option_file: The path to a options file to check
                                     configuration.
                        skip_backup: if True, skip the creation of a backup
                                     file when modifying the options file.
                        server:     Connection information (dict | Server |
                                    str)
    :type kwargs:     dict

    :raise GadgetError:  If the file cannot be updated.

    :return: True if the options file was modified.
    :rtype: boolean
    """

    _LOGGER.info("")
    verbose = kwargs.get("verbose", False)
    update = kwargs.get("update", False)
    option_file = kwargs.get("option_file", None)
    skip_backup = kwargs.get("skip_backup", False)
    server_info = kwargs.get("server", None)

    # Get the server instance
    server = get_server(server_info=server_info)

    # This method requires at least one of the server or the option file.
    if (option_file is None or option_file == "") and server is None:
        raise GadgetError("Either the server or the defaults file was not "
                          "given.")

    if option_file:
        msg = "Running {0} command for file '{1}'.".format(CHECK, option_file)
    else:
        msg = "Running {0} command.".format(CHECK)
    _LOGGER.step(msg)

    try:
        # if server already belongs to a group and the update option
        # was provided, dump its GR configurations to the option file
        if is_member_of_group(server) and is_active_member(server):
            gr_configs = get_gr_configs_from_instance(server)
            if update:
                # the variable comes from the server, no need to test for
                # loose prefix variant.
                if not gr_configs.get("group_replication_group_seeds", None):
                    _LOGGER.warning(
                        "The 'group_replication_group_seeds' is not defined "
                        "on %s. This option is mandatory to allow the server "
                        "to automatically rejoin the cluster after reboot. "
                        "Please manually update its value on option file "
                        "'%s'.", str(server), option_file)
                _LOGGER.info("Updating option file '%s' with Group "
                             "Replication settings from "
                             "%s", option_file, str(server))
                result = persist_gr_config(option_file, gr_configs)
            else:
                result = False
        # If the server doesn't belong to any group, check if it meets GR
        # requirements, printing to stdout what needs to be changed and
        # update the configuration file if provided
        else:
            # The dictionary with the requirements to be verified.
            if server is not None:
                req_dict = get_req_dict(server, None, peer_server=None,
                                        option_file=option_file)
                skip_schema_checks = False
            else:
                req_dict = get_req_dict_for_opt_file(option_file)
                skip_schema_checks = True

            _LOGGER.step("Checking Group Replication "
                        "prerequisites.")

            # set dry_run to avoid changes on server as replication user
            # creation.
            result = check_server_requirements(
                server, req_dict, None, verbose=verbose, dry_run=True,
                skip_schema_checks=skip_schema_checks, update=update,
                skip_backup=skip_backup)

            # verify the group replication is installed and not disabled.
            if server is not None:
                check_gr_plugin_is_installed(server, option_file,
                                             dry_run=(not update))

    finally:
        # Disconnect the server prior to end method invocation.
        if server is not None:
            server.disconnect()

    return result


def start(server_info, **kwargs):
    """Start a group replication group with the given server information.

    :param server_info: Connection information
    :type  server_info: dict | Server | str
    :param kwargs:      Keyword arguments:
                        gr_address: The host:port that the gcs plugin uses to
                                    other servers communicate with this server.
                        dry_run:    Do not make changes to the server
                        verbose:    If the command should show more verbose
                                    information.
                        option_file: The path to a options file to check
                                     configuration.
                        skip_backup: if True, skip the creation of a backup
                                     file when modifying the options file.
                        skip_schema_checks: Skip schema validation.
                        ssl_mode: SSL mode to be used with group replication.
    :type kwargs:       dict

    :raise GadgetError:         If server_info is None.
    :raise GadgetCnxInfoError:  If the connection information on
                                server_info could not be parsed.
    :raise GadgetServerError:   If a connection fails.

    :return: True if the start_gr_plugin method was executed otherwise False.
    :rtype: boolean
    """

    server = get_server(server_info=server_info)
    if server is None:
        raise GadgetError(_ERROR_NO_SERVER)
    msg = _RUNNING_COMMAND.format(START, server)
    _LOGGER.info("")
    _LOGGER.step(msg)

    verbose = kwargs.get("verbose", False)
    dry_run = kwargs.get("dry_run", False)
    gr_host = kwargs.get("gr_address", None)
    skip_schema_checks = kwargs.get("skip_schema_checks", [])
    option_file = kwargs.get("option_file", None)
    skip_backup = kwargs.get("skip_backup", False)

    _LOGGER.step("Checking Group Replication "
                                      "prerequisites.")
    try:
        # Throw an error in case server doesn't support SSL and the ssl_mode
        # option was provided a value other than DISABLED
        if server.select_variable(HAVE_SSL) != 'YES' and \
                kwargs["ssl_mode"] != GR_SSL_DISABLED:
            raise GadgetError(_ERROR_NO_HAVE_SSL.format(server))

        rpl_user_dict = get_rpl_usr(kwargs)

        req_dict = get_req_dict(server, rpl_user_dict["replication_user"],
                                option_file=option_file)

        check_server_requirements(server, req_dict, rpl_user_dict, verbose,
                                  dry_run, skip_schema_checks,
                                  skip_backup=skip_backup,
                                  var_change_warning=True)

        gr_host, local_port = resolve_gr_local_address(gr_host, server.host,
                                                       server.port)

        # verify the group replication is not Disabled in the server.
        check_gr_plugin_is_installed(server, option_file, dry_run)

        # verify the server does not belong already to a GR group.
        if is_active_member(server):
            health(server, **kwargs)
            raise GadgetError(_ERROR_ALREADY_A_MEMBER.format(server, START))

        local_address = "{0}:{1}".format(gr_host, local_port)

        option_parser = req_dict.get(OPTION_PARSER, None)
        gr_config_vars = get_gr_config_vars(local_address, kwargs,
                option_parser,
                server_id=server.select_variable("server_id"))

        if gr_config_vars[GR_GROUP_SEEDS] is None:
            gr_config_vars.pop(GR_GROUP_SEEDS)

        # Remove IP whitelist variable if not set (by user or from the option
        # file) to use the default server value and not set it with None.
        if gr_config_vars[GR_IP_WHITELIST] is None:
            gr_config_vars.pop(GR_IP_WHITELIST)

        if gr_config_vars[GR_GROUP_NAME] is None:
            new_uuid = get_group_uuid_name(server)
            _LOGGER.debug("A new UUID has been generated for the group "
                          "replication name %s", new_uuid)
            gr_config_vars[GR_GROUP_NAME] = new_uuid

        # Set the single_primary mode, if no value given then set ON as
        # default.
        if gr_config_vars[GR_SINGLE_PRIMARY_MODE] is None:
            gr_config_vars[GR_SINGLE_PRIMARY_MODE] = '"ON"'

        _LOGGER.step("Group Replication group name: %s",
                    gr_config_vars[GR_GROUP_NAME])
        setup_gr_config(server, gr_config_vars, dry_run=dry_run)

        # Run the change master to store MySQL replication user name or
        # password information in the master info repository
        do_change_master(server, rpl_user_dict, dry_run=dry_run)

        set_bootstrap(server, dry_run)

        _LOGGER.step("Attempting to start the Group Replication group...")

        # Attempt to start the Group Replication plugin
        start_gr_plugin(server, dry_run)

        # Wait for the super_read_only to be unset.
        super_read_only = server.select_variable("super_read_only", 'global')
        _LOGGER.debug("super_read_only: %s", super_read_only)
        waiting_time = 0
        informed = False
        while int(super_read_only) and waiting_time < TIME_OUT:
            time.sleep(WAIT_SECONDS)
            waiting_time += WAIT_SECONDS
            _LOGGER.debug("have been waiting %s seconds", waiting_time)
            # inform what are we waiting for
            if waiting_time >= 10 and not informed:
                _LOGGER.info("Waiting for super_read_only to be unset.")
                informed = True

            super_read_only = server.select_variable("super_read_only",
                                                     'global')
            _LOGGER.debug("super_read_only: %s", super_read_only)

        if int(super_read_only):
            raise GadgetError("Timeout waiting for super_read_only to be "
                              "unset after call to start Group Replication "
                              "plugin.")

        _LOGGER.step(
                    "Group Replication started for group: %s.",
                    gr_config_vars[GR_GROUP_NAME])

        unset_bootstrap(server, dry_run)

        # Update the Group Replication options on defaults file.
        # Note: Set group_replication_start_on_boot=ON
        if OPTION_PARSER in req_dict.keys():
            persist_gr_config(req_dict[OPTION_PARSER], gr_config_vars,
                              dry_run=dry_run, skip_backup=skip_backup)

        if dry_run:
            _LOGGER.warning(_WARN_DRY_RUN_USED)
            return False

    finally:
        # Disconnect the server prior to end method invocation.
        if server is not None:
            server.disconnect()

    return True


def health(server_info, **kwargs):
    """Display Group Replication health/status information of a server.

    :param server_info: Connection information
    :type  server_info: dict | Server | str
    :param kwargs:      Keyword arguments:
                        verbose:    If the command should show more verbose
                                    information.
                        detailed:   Show extra health info for the server.
    :type kwargs:       dict

    :raise GadgetError:  If server_info is None or belongs to a server
                         that is not a member of Group Replication.
    :raise GadgetCnxInfoError:  If the connection information on
                                server_info could not be parsed.
    :raise GadgetServerError:   If a connection fails.
    """

    verbose = kwargs.get("verbose", False)
    detailed = kwargs.get("detailed", False)

    query_options = {
        "columns": True,
    }
    server = get_server(server_info=server_info)
    if server is None:
        raise GadgetError(_ERROR_NO_SERVER)

    _LOGGER.info("")
    msg = _RUNNING_COMMAND.format((STATUS if detailed else HEALTH), server)
    _LOGGER.step(msg)

    try:
        # SELECT privilege is required for check for membership
        if not is_member_of_group(server):
            raise GadgetError("The server '{0}' is not a member of a GR "
                              "group.".format(server))

        # Retrieve members information
        columns = [MEMBER_ID, MEMBER_HOST, MEMBER_PORT, MEMBER_STATE]
        res = server.exec_query("SELECT {0} FROM {1}"
                                "".format(", ".join(columns),
                                          REP_GROUP_MEMBERS_TABLE),
                                query_options)

        # If no info could be retrieve raise and error
        if len(res[1]) == 0:
            _LOGGER.debug("Cannot retrieve any value from table %s",
                          REP_GROUP_MEMBERS_TABLE)
            if server is not None:
                server.disconnect()
            raise GadgetError(_ERROR_NOT_A_MEMBER.format(server))

        total_members = 0
        online_members = 0

        # Log members information
        _LOGGER.info("Group Replication members: ")
        for results_set in res[1]:
            member_dict = OrderedDict(zip(res[0], results_set))
            if detailed:
                member_dict["ID"] = member_dict.pop(MEMBER_ID)
            else:
                member_dict.pop(MEMBER_ID)
            member_dict["HOST"] = member_dict.pop(MEMBER_HOST)
            member_dict["PORT"] = member_dict.pop(MEMBER_PORT)
            member_dict["STATE"] = member_dict.pop(MEMBER_STATE)
            total_members += 1
            if member_dict["STATE"] == "ONLINE":
                online_members += 1
            _report_dict(member_dict, "{0:<4}{1}: {2}", "ID" if detailed
                         else "HOST", "{0:<2}- {1}: {2}",
                         show_empty_values=verbose)

        _LOGGER.info("")

        # The detailed information is logged as the full HEALTH command.
        if not verbose and not detailed:
            return

        # Retrieve the (replication channels) applier and retriever threads
        # status
        columns = ["m.{0}".format(MEMBER_ID), MEMBER_HOST, MEMBER_PORT,
                   MEMBER_STATE, VIEW_ID, COUNT_TRANSACTIONS_IN_QUEUE,
                   COUNT_TRANSACTIONS_CHECKED, COUNT_CONFLICTS_DETECTED,
                   COUNT_TRANSACTIONS_ROWS_VALIDATING,
                   TRANSACTIONS_COMMITTED_ALL_MEMBERS,
                   LAST_CONFLICT_FREE_TRANSACTION]
        res = server.exec_query("SELECT {0} FROM {1} as m JOIN {2} as s "
                                "on m.MEMBER_ID = s.MEMBER_ID"
                                "".format(", ".join(columns),
                                          REP_GROUP_MEMBERS_TABLE,
                                          REP_MEMBER_STATS_TABLE),
                                query_options)

        # Log the member information of the server that is owner of the
        # following stats, current status of applier and retriever threads
        output = "Server stats: "
        _LOGGER.info(output)
        res_pairs = OrderedDict(zip(res[0], res[1][0]))

        # Log member information
        output = "{0:<2}{1}".format("", "Member")
        _LOGGER.info(output)
        member_dict = OrderedDict()
        member_dict["HOST"] = res_pairs.pop(MEMBER_HOST)
        member_dict["PORT"] = res_pairs.pop(MEMBER_PORT)
        member_dict["STATE"] = res_pairs.pop(MEMBER_STATE)
        member_dict[VIEW_ID] = res_pairs.pop(VIEW_ID)
        member_dict["ID"] = res_pairs.pop(MEMBER_ID)

        _report_dict(member_dict, "{0:<4}{1}: {2}", show_empty_values=verbose)

        # Log Transactions stats
        output = "{0:<2}{1}".format("", "Transactions")
        _LOGGER.info(output)
        trans_dict = OrderedDict()
        trans_dict["IN_QUEUE"] = res_pairs.pop(COUNT_TRANSACTIONS_IN_QUEUE)
        trans_dict["CHECKED"] = res_pairs.pop(COUNT_TRANSACTIONS_CHECKED)
        trans_dict["CONFLICTS_DETECTED"] = \
            res_pairs.pop(COUNT_CONFLICTS_DETECTED)
        trans_dict["VALIDATING"] = res_pairs.pop(
            COUNT_TRANSACTIONS_ROWS_VALIDATING)
        trans_dict["LAST_CONFLICT_FREE"] = res_pairs.pop(
            LAST_CONFLICT_FREE_TRANSACTION)
        trans_dict["COMMITTED_ALL_MEMBERS"] = res_pairs.pop(
            TRANSACTIONS_COMMITTED_ALL_MEMBERS)
        _report_dict(trans_dict, "{0:<4}{1}: {2}", show_empty_values=verbose)

        _report_dict(res_pairs, "{0:<2}{1}: {2}", show_empty_values=verbose)

        res = server.exec_query(
            "SELECT * FROM {0} as c "
            "JOIN {1} as a ON c.channel_name = a.channel_name "
            "ORDER BY c.channel_name, a.worker_id"
            "".format(REP_CONN_STATUS_TABLE, REP_APPLIER_STATUS_BY_WORKER),
            query_options)

        # Log connection stats of the channels
        output = "{0:<2}{1}".format("", "Connection status")
        _LOGGER.info(output)
        for results_set in res[1]:
            res_pairs = OrderedDict(zip(res[0], results_set))
            errors_dict = OrderedDict()
            errors_dict["NUMBER"] = res_pairs.pop("LAST_ERROR_NUMBER")
            errors_dict["MESSAGE"] = res_pairs.pop("LAST_ERROR_MESSAGE")
            errors_dict["TIMESTAMP"] = res_pairs.pop("LAST_ERROR_TIMESTAMP")

            _report_dict(res_pairs, "{0:<4}{1}: {2}", "CHANNEL_NAME",
                         "{0:<2}- {1}: {2}", show_empty_values=verbose)

            if "0000-00-00" not in errors_dict["TIMESTAMP"] or verbose:
                output = "{0:<4}{1}".format("", "Last error")
                _LOGGER.info(output)
                _report_dict(errors_dict, "{0:<6}{1}: {2}",
                             show_empty_values=verbose)
            else:
                output = "{0:<4}{1}".format("", "Last error: None")
                _LOGGER.info(output)
            _LOGGER.info("")
        if online_members < 3:
            plural = "s" if online_members > 1 else ""
            output = ("The group is currently not HA because it has {0} "
                      "ONLINE member{1}.".format(online_members, plural))
            _LOGGER.warning(output)
        _LOGGER.info("")

    finally:
        # Disconnect the server prior to end method invocation.
        if server is not None:
            server.disconnect()

    return


# pylint: disable=R0915
def join(server_info, peer_server_info, **kwargs):
    """Add a server to an existing Group Replication group.

    The contact point to add the new server to the group replication is the
    specified peer server, which must be already a member of the group.

    :param server_info: Connection information
    :type  server_info: dict | Server | str
    :param peer_server_info: Connection information of a server member of a
                             Group Replication group.
    :type  peer_server_info: dict | Server | str
    :param kwargs:  Keyword arguments:
                        gr_address: The host:port that the gcs plugin uses to
                                    other servers communicate with this server.
                        dry_run:    Do not make changes to the server
                        verbose:    If the command should show more verbose
                                    information.
                        option_file: The path to a options file to check
                                     configuration.
                        skip_backup: if True, skip the creation of a backup
                                     file when modifying the options file.
                        ssl_mode: SSL mode to be used with group replication.
                                  (Note server GR SSL modes need
                                  to be consistent with the SSL GR modes on the
                                  peer-server otherwise an error will be
                                  thrown).
                        skip_rpl_user: If True, skip the creation of the
                                       replication user.
    :type kwargs:   dict

    :raise GadgetError:         If server_info or peer_server_info is None.
                                If the given peer_server_info is from a server
                                that is not a member of Group Replication.
    :raise GadgetCnxInfoError:  If the connection information on server_info
                                or peer_server_info could not be parsed.
    :raise GadgetServerError:   If a connection fails.

    :return: True if the start_gr_plugin method was executed otherwise False.
    :rtype: boolean
    """

    verbose = kwargs.get("verbose", False)
    dry_run = kwargs.get("dry_run", False)
    gr_host = kwargs.get("gr_address", None)
    option_file = kwargs.get("option_file", None)
    skip_backup = kwargs.get("skip_backup", False)
    # Default is value for ssl_mode is REQUIRED
    ssl_mode = kwargs.get("ssl_mode", GR_SSL_REQUIRED)
    skip_rpl_user = kwargs.get("skip_rpl_user", False)

    # Connect to the server
    server = get_server(server_info=server_info)
    if server is None:
        raise GadgetError(_ERROR_NO_SERVER)

    msg = _RUNNING_COMMAND.format(JOIN, server)
    _LOGGER.info("")
    _LOGGER.step(msg)

    _LOGGER.step("Checking Group Replication "
                                      "prerequisites.")

    peer_server = get_server(server_info=peer_server_info)

    # Connect to the peer server
    if peer_server is None:
        if server is not None:
            server.disconnect()
        raise GadgetError("No peer server provided. It is required to get "
                          "information from the group.")

    try:
        # verify the peer server belong to a GR group.
        if not is_member_of_group(peer_server):
            raise GadgetError("Peer server '{0}' is not a member of a GR "
                              "group.".format(peer_server))

        # verify the server status is ONLINE
        peer_server_state = get_member_state(peer_server)
        if peer_server_state != 'ONLINE':
            raise GadgetError("Cannot join instance {0}. Peer instance {1} "
                              "state is currently '{2}', but is expected to "
                              "be 'ONLINE'.".format(server, peer_server,
                                                    peer_server_state))

        # Throw an error in case server doesn't support SSL and the ssl_mode
        # option was provided a value other than DISABLED
        if server.select_variable(HAVE_SSL) != 'YES' and \
           ssl_mode != GR_SSL_DISABLED:
            raise GadgetError(_ERROR_NO_HAVE_SSL.format(server))

        # Throw an error in case there is any SSL incompatibilities are found
        # on the peer server or if the peer-server is incompatible with the
        # value of the ssl_mode option.
        check_peer_ssl_compatibility(peer_server, ssl_mode)

        if not skip_rpl_user:
            rpl_user_dict = get_rpl_usr(kwargs)
        else:
            rpl_user_dict = None

        # Do not check/create the replication user in the instance to add,
        # in order to avoid errors if it is in read-only-mode.
        # (GR automatically enables super-read-only when stopping the
        # plugin, starting with version 8.0.2)
        req_dict = get_req_dict(server, None, peer_server,
                                option_file=option_file)

        check_server_requirements(server, req_dict, rpl_user_dict, verbose,
                                  dry_run, skip_backup=skip_backup,
                                  var_change_warning=True)

        # verify the group replication is installed and not disabled.
        check_gr_plugin_is_installed(server, option_file, dry_run)

        # Initialize log error access and get current position in it
        error_log_size = None
        if server.is_alias("127.0.0.1"):
            error_log = LocalErrorLog(server)
            try:
                error_log_size = error_log.get_size()
            except Exception as err:  # pylint: disable=W0703
                _LOGGER.warning(
                    "Unable to access the server error log: %s", str(err))
        else:
            _LOGGER.warning("Not running locally on the server and can not "
                            "access its error log.")

        # verify the server does not belong already to a GR group.
        if is_active_member(server):
            health(server, **kwargs)
            raise GadgetError(_ERROR_ALREADY_A_MEMBER.format(server, JOIN))

        gr_host, local_port = resolve_gr_local_address(gr_host, server.host,
                                                       server.port)

        local_address = "{0}:{1}".format(gr_host, local_port)
        _LOGGER.debug("local_address to use: %s", local_address)

        # Get local_address from the peer server to add to the list of
        # group_seeds.
        peer_local_address = get_gr_local_address_from(peer_server)

        if peer_server.select_variable(
            "group_replication_single_primary_mode") in ('1', 'ON'):
            kwargs["single_primary"] = "ON"

        option_parser = req_dict.get(OPTION_PARSER, None)
        gr_config_vars = get_gr_config_vars(local_address, kwargs,
                            option_parser, peer_local_address,
                            server_id=server.select_variable("server_id"))

        # The following code has been commented because the logic to create
        # the replication-user has been moved to the Shell c++ code.
        # The code wasn't removed to serve as knowledge base for the MP
        # refactoring to C++

        # Do several replication user related tasks if the
        # skip-replication-user option was not provided
        #if not skip_rpl_user:
            # The replication user for be check/create on the peer server.
            # NOTE: rpl_user_dict["host"] has the FQDN resolved from the host
            # provided by the user
        #    replication_user = "{0}@'{1}'".format(
        #        rpl_user_dict["recovery_user"], rpl_user_dict["host"])
        #    rpl_user_dict["replication_user"] = replication_user

            # Check the given replication user exists on peer
        #    req_dict_user = get_req_dict_user_check(peer_server,
        #                                            replication_user)

            # Check and create the given replication user on peer server.
            # NOTE: No other checks will be performed, only the replication
            # user.
        #    check_server_requirements(peer_server, req_dict_user,
        #                              rpl_user_dict, verbose, dry_run,
        #                              skip_schema_checks=True)

        # IF the group name is not set, try to acquire it from a peer server.
        if gr_config_vars[GR_GROUP_NAME] is None:
            _LOGGER.debug("Trying to retrieve group replication name from "
                          "peer server.")
            group_name = get_gr_name_from_peer(peer_server)

            _LOGGER.debug("Retrieved group replication name from peer"
                          " server: %s.", group_name)
            gr_config_vars[GR_GROUP_NAME] = group_name

        # Set the single_primary mode according to the value set on peer
        # server
        if gr_config_vars[GR_SINGLE_PRIMARY_MODE] is None:
            gr_config_vars[GR_SINGLE_PRIMARY_MODE] = \
                get_gr_variable_from_peer(peer_server, GR_SINGLE_PRIMARY_MODE)

        if gr_config_vars[GR_GROUP_NAME] is None:
            raise GadgetError(
                _ERROR_UNABLE_TO_GET.format("Group Replication group name",
                                            peer_server))

        _LOGGER.step(
                    "Joining Group Replication group: %s",
                    gr_config_vars[GR_GROUP_NAME])

        if gr_config_vars[GR_GROUP_SEEDS] is None:
            raise GadgetError(
                _ERROR_UNABLE_TO_GET.format("peer addresses", peer_server))

        # Remove IP whitelist variable if not set (by user or from the option
        # file) to use the default server value and not set it with None.
        if gr_config_vars[GR_IP_WHITELIST] is None:
            gr_config_vars.pop(GR_IP_WHITELIST)

        setup_gr_config(server, gr_config_vars, dry_run=dry_run)

        if not skip_rpl_user:
            # if the skip replication user option was not specified,
            # run the change master to store MySQL replication user name or
            # password information in the master info repository
            do_change_master(server, rpl_user_dict, dry_run=dry_run)

        _LOGGER.step(
                    "Attempting to join to Group Replication group...")

        try:
            start_gr_plugin(server, dry_run)
            _LOGGER.step("Server %s joined "
                        "Group Replication group %s.", server,
                        gr_config_vars[GR_GROUP_NAME])

        except:
            _LOGGER.error("\nGroup Replication join failed.")
            if error_log_size is not None:
                log_data = error_log.read(error_log_size)
                if log_data:
                    _LOGGER.error("Group Replication plugin failed to start. "
                                  "Server error log contains the following "
                                  "errors: \n %s", log_data)
            raise

        # Update the Group Replication options on defaults file.
        # Note: Set group_replication_start_on_boot=ON
        if OPTION_PARSER in req_dict.keys():
            persist_gr_config(req_dict[OPTION_PARSER], gr_config_vars,
                              dry_run=dry_run, skip_backup=skip_backup)

        if dry_run:
            _LOGGER.warning(_WARN_DRY_RUN_USED)
            return False

    finally:
        # disconnect servers.
        if server is not None:
            server.disconnect()
        if peer_server is not None:
            peer_server.disconnect()

    return True


def leave(server_info, **kwargs):
    """Removes a server from a Group Replication group.

    :param server_info: Connection information
    :type  server_info: dict | Server | str
    :param kwargs:      Keyword arguments:
                        dry_run:    Do not make changes to the server
                        option_file: The path to a options file to check
                                     configuration.
                        skip_backup: if True, skip the creation of a backup
                                     file when modifying the options file.
    :type kwargs:       dict

    :raise GadgetError: If the given server is not a member of a Group
                        Replication

    :return: True if the server stop_gr_plugin command was executed otherwise
             False.
    :rtype: boolean
    """

    # get the server instance
    server = get_server(server_info=server_info)

    dry_run = kwargs.get("dry_run", False)
    option_file = kwargs.get("option_file", None)
    skip_backup = kwargs.get("skip_backup", False)

    if server is None:
        raise GadgetError(_ERROR_NO_SERVER)

    msg = _RUNNING_COMMAND.format(LEAVE, server)
    _LOGGER.info("")
    _LOGGER.step(msg)

    try:
        # verify server status (is in a group?)
        if is_member_of_group(server) and is_active_member(server):
            _LOGGER.step("Attempting to leave from the "
                        "Group Replication group...")
            if not dry_run:
                stop_gr_plugin(server)
            _LOGGER.info("Server state: %s", get_member_state(server))
            _LOGGER.step("Server %s has left the group.",
                        server)

            # Update the Group Replication options on defaults file.
            # Note: Set group_replication_start_on_boot=ON
            if option_file is not None and option_file != "":
                persist_gr_config(option_file, None, set_on=False,
                                  dry_run=dry_run, skip_backup=skip_backup)

            return True

        # server is not active
        elif is_member_of_group(server):
            _LOGGER.warning("The server %s is not actively replicating.",
                            server)
            _LOGGER.info("Server state: %s", get_member_state(server))
            _LOGGER.step("Server %s is "
                        "not active in the group.", server)

            # Update the group_replication_start_on_boot option on defaults
            # file.
            # Note: Set group_replication_start_on_boot=OFF
            if option_file is not None and option_file != "":
                persist_gr_config(option_file, None, set_on=False,
                                  dry_run=dry_run, skip_backup=skip_backup)

        # the server has not been configured for GR
        else:
            raise GadgetError(_ERROR_NOT_A_MEMBER.format(server))

        if dry_run:
            _LOGGER.warning(_WARN_DRY_RUN_USED)

    finally:
        # disconnect servers.
        if server is not None:
            server.disconnect()

    return False
