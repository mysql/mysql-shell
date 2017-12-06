#
# Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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
This module contains and abstraction of a MySQL user object.
"""

import re
import logging

from collections import namedtuple, defaultdict

from mysql_gadgets.exceptions import (GadgetError,
                                      GadgetQueryError)
from mysql_gadgets.common.connection_parser import (clean_IPv6,
                                                    parse_user_host,)
from mysql_gadgets.common.logger import CustomLevelLogger
from mysql_gadgets.common.sql_utils import (is_quoted_with_backticks,
                                            quote_with_backticks,)


ERROR_USER_WITHOUT_PRIVILEGES = ("User '{user}' on '{host}@{port}' does not "
                                 "have sufficient privileges to "
                                 "{operation} (required: {req_privileges}).")

logging.setLoggerClass(CustomLevelLogger)
_LOGGER = logging.getLogger(__name__)


def change_user_privileges(server, user_name, host, user_passwd=None,
                           grant_list=None, revoke_list=None,
                           disable_binlog=False, create_user=False,
                           with_grant=False):
    """ Change the privileges of a new or existing user.

    This method GRANT or REVOKE privileges to a new user (creating it) or
    existing user.

    :param server:          Server instance to apply the changes
    :type server:           Server (mysql_gadgets.common.server.Server)
    :param user_name:       User name to apply changes.
    :type user_name:        string
    :param host:            Host name associated to the user account.
    :type host:             string
    :param user_passwd:     User's password.
    :type user_passwd:      string
    :param grant_list:      List of privileges to GRANT.
    :type grant_list:       list of strings
    :param revoke_list:     List of privileges to REVOKE.
    :type revoke_list:      list of strings
    :param disable_binlog:  Indicate if binary logging will be disabled to
                            perform this operation (and re-enabled at the end).
                            By default: False (do not disable binary logging).
    :type disable_binlog:   boolean
    :param create_user:     Indicate if the user will be created before
                            changing its privileges. By default: False (do not
                            create user).
    :type create_user:      boolean
    :param with_grant:      Allow the user to grant the privileges granted.
                            By default: False (do not grant privileges with
                            grant option).
    :type with_grant:       boolean
    """
    if disable_binlog:
        server.exec_query("SET SQL_LOG_BIN=0")
    if create_user:
        server.exec_query("CREATE USER '{0}'@'{1}'".format(user_name, host))
    if user_passwd is not None:
        query = "ALTER USER '{0}'@'{1}' IDENTIFIED BY '{2}'"
        server.exec_query(query.format(user_name, host, user_passwd),
                          query_to_log=query.format(user_name, host,
                                                    "*******"))
    if grant_list:
        if with_grant:
            grant_option = "WITH GRANT OPTION"
        else:
            grant_option = ""
        grants_str = ", ".join(grant_list)
        server.exec_query("GRANT {0} ON *.* TO '{1}'@'{2}' {3}"
                          "".format(grants_str, user_name, host, grant_option))

    if revoke_list:
        revoke_str = ", ".join(revoke_list)
        server.exec_query("REVOKE {0} ON *.* FROM '{1}'@'{2}'"
                          "".format(revoke_str, user_name, host))
    if disable_binlog:
        server.exec_query("SET SQL_LOG_BIN=1")


def check_privileges(server, operation, privileges, description=None):
    """Check required privileges.

    This method check if the used user possess the required privileges to
    execute a statement or operation.
    An exception is thrown if the user doesn't have enough privileges.

    :param server:      Server instance to check.
    :type  server:      Server
    :param operation:   The name of tha task that requires the privileges,
                        used in the error message if an exception is thrown.
    :type  operation:   string
    :param privileges:  List of the required privileges.
    :type  privileges:  List of strings

    :param description:  Description of the operation requiring the User's
                         privileges, if given it will be used in the loggin
                         message.
    :type  description:  string

    :raise GadgetError: if the user lacks one or more required privileges.
    """
    # log message if description was given.
    if description is not None:
        _LOGGER.info("Checking user permission to %s...\n", description)

    # Check privileges
    user_obj = User(server, "{0}@{1}".format(server.user, server.host))

    privileges_needed = user_obj.check_missing_privileges(privileges)

    if len(privileges_needed) > 0:
        raise GadgetError(ERROR_USER_WITHOUT_PRIVILEGES.format(
            user=server.user, host=server.host, port=server.port,
            operation=operation, req_privileges=privileges_needed
        ))


class User(object):
    """
    The User class can be used to perform the following tasks:

        - Parsing user@host:passwd strings
        - Create, Drop user
        - Check to see if user exists
        - Retrieving grants for user
    """

    def __init__(self, server1, user, passwd=None, verbosity=0):
        """Constructor

        :param server1:    Server instance
        :type server1:     Server
        :param user:       MySQL user account name (user@host).
        :type user:        string
        :param passwd:     Password for the user. By default, None
                           (no password).
        :type passwd:      string
        :param verbosity:  Verbosity level to log extra information during
                           operations. By default, 0 (no extra information
                           logged).
        :type verbosity:   integer
        """

        self.server1 = server1
        self.sql_mode = ''
        self.user, self.host = parse_user_host(user)
        self.host = clean_IPv6(self.host)
        self.passwd = passwd
        self.verbosity = verbosity
        self.current_user = None
        self.grant_dict = None
        self.global_grant_dict = None
        self.grant_list = None
        self.global_grant_list = None
        self.query_options = {
            'fetch': False
        }

    def create(self, new_user=None, passwd=None, ssl=False,
               disable_binlog=False):
        """Create the user

        Attempts to create the user.

        :param new_user:   MySQL user string (user@host)
                           (optional) If omitted, operation is performed
                           on the class instance user name.
        :type new_user:    string in the form "user@host".
        :param passwd:     password for user to create (optional).
        :type passwd:      string.
        :param ssl:        If True the grant will include 'REQUIRE SSL'
                           (Default False).
        :type ssl:         Boolean, if True set user requires ssl to login.
        :param disable_binlog: Turn of the binary while creating the user.
        :type disable_binlog: Boolean, if True turn off the binary log.
        """
        query_str = "CREATE USER IF NOT EXISTS "
        if new_user:
            user, host = parse_user_host(new_user)
            query_str += "'{0}'@'{1}' ".format(user, host)
        else:
            query_str += "'{0}'@'{1}' ".format(self.user, self.host)
            passwd = self.passwd

        if passwd:
            query_str += "IDENTIFIED BY '{0}'".format(passwd)

        if ssl:
            query_str = "{0} {1}".format(query_str, " REQUIRE SSL")

        # Turn off binlog
        binlog_disabled = False
        if disable_binlog and self.server1.binlog_enabled():
            self.server1.toggle_binlog(action="disable")
            binlog_disabled = True
        # Create the user at the server
        query_to_log = query_str
        if passwd:
            query_to_log = query_str.replace(
                "IDENTIFIED BY '{0}'".format(passwd),
                "IDENTIFIED BY '******'")
        self.server1.exec_query(query_str, self.query_options,
                                query_to_log=query_to_log)
        # Turn ON binlog only if it was disabled before
        if disable_binlog and binlog_disabled:
            self.server1.toggle_binlog(action="enable")

    def drop(self, new_user=None):
        """Drop user from the server

        Attempts to drop the user.

        :param new_user:  MySQL user account name (user@host) to drop.
                          If omitted then the operation is performed using
                          the user name value defined for the User instance.
        :type new_user:   string

        :return: True if the user was drop successfully and False if an error
                 occurred (could not drop user).
        :rtype:  boolean
        """
        query_str = "DROP USER "
        if new_user:
            user, host = parse_user_host(new_user)
            query_str += "'{0}'@'{1}' ".format(user, host)
        else:
            query_str += "'{0}'@'{1}' ".format(self.user, self.host)

        if self.verbosity > 0:
            _LOGGER.debug(query_str)

        try:
            self.server1.exec_query(query_str, self.query_options)
        except GadgetError:
            return False
        return True

    def exists(self, user_name=None):
        """Check to see if the user exists

        :param user_name:  MySQL user account name (user@host) to check.
                           If omitted then the operation is performed using
                           the user name value defined for the User instance.
        :type user_name:   string

        :return: True if user exists, and False if the user does not exist.
        :rtype:  boolean
        """

        user, host = self.user, self.host
        if user_name:
            user, host = parse_user_host(user_name)

        res = self.server1.exec_query("SELECT * FROM mysql.user "
                                      "WHERE user = %s and host = %s",
                                      {'params': (user, host)})

        return res is not None and len(res) >= 1

    @staticmethod
    def _get_grants_as_dict(grant_list, verbosity=0, sql_mode=''):
        """Transforms list of grant string statements into a dictionary.

        :param grant_list:  List of grant strings as returned from the server
        :type grant_list:   list
        :param verbosity:   Verbosity level to log extra information during
                            operations. By default, 0 (no extra information
                            logged).
        :type verbosity:    integer
        :param sql_mode:    The sql_mode set on the server.
        :type sql_mode:     string

        :return: A default_dict with the grant information
        :rtype: dict
        """
        grant_dict = defaultdict(lambda: defaultdict(set))
        for grant in grant_list:
            grant_tpl = User.parse_grant_statement(grant[0], sql_mode)
            # Ignore PROXY privilege, it is not yet supported
            if verbosity > 0:
                if 'PROXY' in grant_tpl[0]:
                    _LOGGER.warning("PROXY privilege will be ignored.")
            grant_tpl.privileges.discard('PROXY')
            if grant_tpl.privileges:
                grant_dict[grant_tpl.db][grant_tpl.object].update(
                    grant_tpl.privileges)
        return grant_dict

    # pylint: disable=too-many-return-statements
    def get_grants(self, globals_privs=False, as_dict=False, refresh=False):
        """Retrieve the grants for the current user

        :param globals_privs:  Include global privileges in clone (i.e. user@%)
        :type globals_privs:   boolean
        :param as_dict:        If True, instead of a list of plain grant
                               strings, return a dictionary with the grants.
        :type as_dict:         boolean
        :param refresh:        If True, reads grant privileges directly from
                               the server and updates cached values, otherwise
                               uses the cached values.
        :type refresh:         boolean

        :return: result set or None if no grants defined
        :rtype: List, dict or None
        """

        # only read values from server if needed
        if refresh or not self.grant_list or not self.global_grant_list:
            # Get the users' connection user@host if not retrieved
            if self.current_user is None:
                res = self.server1.exec_query("SELECT CURRENT_USER()")
                parts = res[0][0].split('@')
                # If we're connected as some other user, use the user@host
                # defined at instantiation
                if parts[0] != self.user:
                    host = clean_IPv6(self.host)
                    self.current_user = "'%s'@'%s'" % (self.user, host)
                else:
                    self.current_user = "'%s'@'%s'" % (parts[0], parts[1])
            grants = []
            try:
                res = self.server1.exec_query("SHOW GRANTS FOR "
                                              "{0}".format(self.current_user))
                for grant in res:
                    grants.append(grant)
            except GadgetQueryError:
                pass  # Error here is ok - no grants found.

            # Cache user grants
            self.grant_list = grants[:]
            self.sql_mode = self.server1.select_variable("SQL_MODE")
            self.grant_dict = User._get_grants_as_dict(self.grant_list,
                                                       self.verbosity,
                                                       self.sql_mode)
            # If current user is already using global host wildcard '%', there
            # is no need to run the show grants again.
            if globals_privs:
                if self.host != '%':
                    try:
                        res = self.server1.exec_query(
                            "SHOW GRANTS FOR '{0}'{1}".format(self.user,
                                                              "@'%'"))
                        for grant in res:
                            grants.append(grant)
                        self.global_grant_list = grants[:]
                        self.global_grant_dict = User._get_grants_as_dict(
                            self.global_grant_list, self.verbosity)
                    except GadgetQueryError:
                        # User has no global privs, return the just the ones
                        # for current host
                        self.global_grant_list = self.grant_list
                        self.global_grant_dict = self.grant_dict
                else:
                    # if host is % then we already have the global privs
                    self.global_grant_list = self.grant_list
                    self.global_grant_dict = self.grant_dict

        if globals_privs:
            if as_dict:
                return self.global_grant_dict
            else:
                return self.global_grant_list
        else:
            if as_dict:
                return self.grant_dict
            else:
                return self.grant_list

    def has_privilege(self, db_, obj, access, allow_skip_grant_tables=True):
        """Check to see user has a specific access to a db_.object.

        :param db_:    Name of database.
        :type db_:     string
        :param obj:    Name of object.
        :type obj:     string
        :param access: MySQL privilege to check (e.g. SELECT, SUPER, DROP).
        :type access:  string
        :param allow_skip_grant_tables:  If True, allow silent failure for
                                         cases where the server is started with
                                         --skip-grant-tables. Default=True
        :type allow_skip_grant_tables:   boolean

        :return: True if user has access, False if not
        :rtype: boolean
        """
        grants_enabled = self.server1.grant_tables_enabled()
        # If grants are disabled and it is Ok to allow skipped grant tables,
        # return True - privileges disabled so user can do anything.
        if allow_skip_grant_tables and not grants_enabled:
            return True
        # Convert privilege to upper cases.
        access = access.upper()

        # Get grant dictionary
        grant_dict = self.get_grants(globals_privs=True, as_dict=True,
                                     refresh=True)

        # If self has all privileges for all databases, no need to check,
        # simply return True
        if ("ALL PRIVILEGES" in grant_dict['*']['*'] and
                "GRANT OPTION" in grant_dict['*']['*']):
            return True

        # Quote db_ and obj with backticks if necessary
        self.sql_mode = ''
        if not is_quoted_with_backticks(db_, self.sql_mode) and db_ != '*':
            db_ = quote_with_backticks(db_, self.sql_mode)

        if not is_quoted_with_backticks(obj, self.sql_mode) and obj != '*':
            obj = quote_with_backticks(obj, self.sql_mode)

        # USAGE privilege is the same as no privileges,
        # so everyone has it.
        if access == "USAGE":
            return True
        # Even if we have ALL PRIVILEGES grant, we might not have WITH GRANT
        # OPTION privilege.
        # Check server wide grants.
        elif (access in grant_dict['*']['*'] or
              "ALL PRIVILEGES" in grant_dict['*']['*'] and
              access != "GRANT OPTION"):
            return True
        # Check database level grants.
        elif (access in grant_dict[db_]['*'] or
              "ALL PRIVILEGES" in grant_dict[db_]['*'] and
              access != "GRANT OPTION"):
            return True
        # Check object level grants.
        elif (access in grant_dict[db_][obj] or
              "ALL PRIVILEGES" in grant_dict[db_][obj] and
              access != "GRANT OPTION"):
            return True
        else:
            return False

    def check_missing_privileges(self, privileges, as_string=True):
        """Checks if this user has the required privileges, returning a list of
        the missing privileges.

        This method check if the used user possess the required privileges to
        execute a statement or operation.
        Returns a a list of the privileges which the given user lacks.

        :param privileges:  List of the required privileges.
        :type  privileges:  list
        :param as_string:   Defines if the result must be returned as a string
                            (True by default) or as a list (False).
        :type as_string:    boolean

        :return: a list of the privileges which the given user lacks.
        :rtype: str or list if as_string is set to False.
        """

        # Check privileges
        need_privileges = []
        for privilege in privileges:
            if not self.has_privilege('*', '*', privilege):
                need_privileges.append(privilege)

        if len(need_privileges) > 0:
            # Always sort, easier to validate
            need_privileges.sort()
            if not as_string:
                return need_privileges
            if len(need_privileges) > 1:
                privileges_needed = "{0} and {1}".format(
                    ", ".join(need_privileges[:-1]),
                    need_privileges[-1]
                )
            else:
                privileges_needed = need_privileges[0]

            return privileges_needed

        if as_string:
            return ""
        else:
            return need_privileges

    @staticmethod
    def parse_grant_statement(statement, sql_mode=''):
        """ Returns a namedtuple with the parsed GRANT information.

        :param statement:  Grant string in the sql format returned by the
                           server.
        :type statement:   string
        :param sql_mode:   The sql_mode set on the server.
        :type sql_mode:    string

        :return: named tuple with GRANT information or None.
        :rtype: Tuple or None
        :raise GadgetError: If it is unable to parse grant statement.
        """

        grant_parse_re = re.compile(r"""
            GRANT\s(.+)?\sON\s # grant or list of grants
            (?:(?:PROCEDURE\s)|(?:FUNCTION\s))? # optional for routines only
            (?:(?:(\*|`?[^']+`?)\.(\*|`?[^']+`?)) # object where grant applies
            | ('[^']*'@'[^']*')) # For proxy grants user/host
            \sTO\s([^@]+@[\S]+) # grantee
            (?:\sIDENTIFIED\sBY\sPASSWORD
             (?:(?:\s<secret>)|(?:\s\'[^\']+\')?))? # optional pwd
            (?:\sREQUIRE\sSSL)? # optional SSL
            (\sWITH\sGRANT\sOPTION)? # optional grant option
            $ # End of grant statement
            """, re.VERBOSE)

        grant_tpl_factory = namedtuple("grant_info", "privileges proxy_user "
                                                     "db object user")
        match = re.match(grant_parse_re, statement)

        if match:
            # quote database name and object name with backticks
            if match.group(1).upper() != 'PROXY':
                db_ = match.group(2)
                if not is_quoted_with_backticks(db_, sql_mode) and db_ != '*':
                    db_ = quote_with_backticks(db_, sql_mode)
                obj = match.group(3)
                if not is_quoted_with_backticks(obj, sql_mode) and obj != '*':
                    obj = quote_with_backticks(obj, sql_mode)
            else:  # if it is not a proxy grant
                db_ = obj = None
            grants = grant_tpl_factory(
                # privileges
                set([priv.strip() for priv in match.group(1).split(",")]),
                match.group(4),  # proxied user
                db_,  # database
                obj,  # object
                match.group(5),  # user
            )
            # If user has grant option, add it to the list of privileges
            if match.group(6) is not None:
                grants.privileges.add("GRANT OPTION")
        else:
            raise GadgetError("Unable to parse grant statement "
                              "{0}".format(statement))

        return grants
