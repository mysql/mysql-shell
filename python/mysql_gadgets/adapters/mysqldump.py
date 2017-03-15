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
"""Adapter module for mysqldump."""

import logging
import os
import subprocess
import shlex

from mysql_gadgets.adapters import (StreamClone, ImageClone, WithValidation,
                                    MYSQL_DEST, MYSQL_SOURCE)
from mysql_gadgets.common.constants import QUOTE_CHAR, PATH_ENV_VAR
from mysql_gadgets.common.server import Server
from mysql_gadgets.common.user import User
from mysql_gadgets.common.tools import get_tool_path
from mysql_gadgets import exceptions


_LOGGER = logging.getLogger(__name__)

_MYSQLDUMP_STREAM_BACKUP_CMD = (
    "{quote}{mysqldump_exec}{quote} --defaults-file={quote}{config_file}"
    "{quote} --all-databases --triggers --events --routines --opt "
    "--flush-privileges --set-gtid-purged=AUTO --ignore-table=mysql.plugin "
    "--ignore-table=mysql.slave_relay_log_info "
    "--ignore-table=mysql.slave_master_info "
    "--ignore-table=mysql.general_log "
    "--ignore-table=mysql.slow_log")

_MYSQLDUMP_STREAM_RESTORE_CMD = (
    "{quote}{mysqlc_exec}{quote} --defaults-file={quote}{config_file}{quote}")

_MYSQLDUMP_IMAGE_BACKUP_CMD = (
    _MYSQLDUMP_STREAM_BACKUP_CMD + " --result-file={quote}{image_file}{quote}")

_MYSQLDUMP_IMAGE_RESTORE_CMD = (
    "{quote}{mysqlc_exec}{quote} --defaults-file={quote}{config_file}{quote} "
    "-e {quote}source {image_file} {quote}"
)


class MySQLDump(StreamClone, ImageClone, WithValidation):
    """"Stream server clone using mysqldump."""
    @staticmethod
    def clone_stream(connection_dict):
        """Clone the contents of source server into destination using a stream.

        :param connection_dict: dictionary of dictionaries of connection
                                information: mysql users and host users. It can
                                have the following keys: MYSQL_SOURCE,
                                MYSQL_DEST, HOST_SOURCE and HOST_DEST.
                                Each of these keys has as a value a dict with
                                the following keys: user, hostname, port,
                                passwd and their respective values.
        :type connection_dict: dict
        """
        # Check tool requirements
        mysqldump_exe = get_tool_path(None, "mysqldump", search_path=True,
                                      required=False)
        if not mysqldump_exe:
            raise exceptions.GadgetError(
                "Could not find mysqldump executable. Make sure it is on "
                "{0}.".format(PATH_ENV_VAR))

        mysqlc_exe = get_tool_path(None, "mysql", search_path=True,
                                   required=False)
        if not mysqlc_exe:
            raise exceptions.GadgetError(
                "Could not find mysql client executable. Make sure it is on "
                "{0}.".format(PATH_ENV_VAR))

        # Creating Server instances for source and destination servers
        source_dict = connection_dict[MYSQL_SOURCE]
        destination_dict = connection_dict[MYSQL_DEST]
        try:
            source_server = Server({'conn_info': source_dict})
        except exceptions.GadgetError as err:
            _LOGGER.error(
                "Unable to create a Server instance for source server."
                "Source dict was: %s", source_dict)
            raise err
        try:
            destination_server = Server({'conn_info': destination_dict})
        except exceptions.GadgetError as err:
            _LOGGER.error("Unable to create a Server instance for destination "
                          "server. Destination dict was: %s", destination_dict)
            raise err

        # Connect to source and destination servers
        try:
            source_server.connect()
        except exceptions.GadgetServerError as err:
            raise exceptions.GadgetError(
                "Unable to connect to source server: {0}".format(str(err)))
        try:
            destination_server.connect()
        except exceptions.GadgetServerError as err:
            raise exceptions.GadgetError("Unable to connect to destination "
                                         "server: {0}.".format(str(err)))

        # Create config_file for mysqldump
        dump_config_file = Server.to_config_file(source_server, "mysqldump")

        # Create config_file for mysql client
        client_config_file = Server.to_config_file(destination_server,
                                                   "client")

        # Create command list to create the backup
        backup_cmd = shlex.split(
            _MYSQLDUMP_STREAM_BACKUP_CMD.format(mysqldump_exec=mysqldump_exe,
                                                config_file=dump_config_file,
                                                quote=QUOTE_CHAR))

        # Create command list to restore the backup
        restore_cmd = shlex.split(
            _MYSQLDUMP_STREAM_RESTORE_CMD.format(
                mysqlc_exec=mysqlc_exe, config_file=client_config_file,
                quote=QUOTE_CHAR
            ))

        # enable global read_lock
        _LOGGER.debug("Locking global read lock on source server to prevent "
                      "modifications during clone.")
        source_server.toggle_global_read_lock(True)
        _LOGGER.debug("Source server locked (read-only=ON)")

        try:
            _LOGGER.debug("Dumping contents of source server using command: "
                          "%s", " ".join(backup_cmd))

            dump_process = subprocess.Popen(backup_cmd, stdout=subprocess.PIPE,
                                            stderr=subprocess.PIPE,
                                            universal_newlines=True)
            _LOGGER.debug("Restoring contents to destination server using "
                          "command: %s", " ".join(restore_cmd))
            restore_process = subprocess.Popen(restore_cmd,
                                               stdin=dump_process.stdout,
                                               stdout=subprocess.PIPE,
                                               stderr=subprocess.PIPE,
                                               universal_newlines=True)
            # We call dump_process.stdout.close before
            # restore_process.communicate so that if restore_process dies
            # prematurely the SIGPIPE signal can be processed by dump_process
            # allowing it to exit.
            dump_process.stdout.close()
            # Wait for restore process to end and get the output.
            _, err = restore_process.communicate()
            dump_process.wait()
            error_msg = ""
            if dump_process.returncode:
                error_msg = (
                    "mysqldump exited with error code '{0}' and message: "
                    "'{1}'. ".format(dump_process.returncode,
                                     dump_process.stderr.read().strip()))
            else:
                _LOGGER.info("Dump process successfully completed.")
            if restore_process.returncode:
                error_msg += (
                    "MySQL client exited with error code '{0}' and message: "
                    "'{1}'".format(restore_process.returncode, err.strip()))
            else:
                _LOGGER.info("Restore process successfully completed.")

            # If there were errors, raise an exception to warn the user.
            if error_msg:
                raise exceptions.GadgetError(error_msg)
        finally:
            # disable global read_lock
            _LOGGER.debug("Unlocking global read lock on source server.")
            source_server.toggle_global_read_lock(False)
            _LOGGER.debug("Source server unlocked. (read-only=OFF)")
            # delete created configuration files
            try:
                _LOGGER.debug("Removing configuration file '%s'",
                              dump_config_file)
                os.remove(dump_config_file)
            except OSError:
                _LOGGER.warning("Unable to remove configuration file '%s'",
                                dump_config_file)
            else:
                _LOGGER.debug("Configuration file '%s' successfully removed",
                              dump_config_file)
            try:
                _LOGGER.debug("Removing configuration file '%s'",
                              client_config_file)
                os.remove(client_config_file)
            except OSError:
                _LOGGER.warning("Unable to remove configuration file '%s'",
                                client_config_file)
            else:
                _LOGGER.debug("Configuration file '%s' successfully removed",
                              client_config_file)

        _LOGGER.info("Contents loaded successfully into destination "
                     "server")

    @staticmethod
    def backup_to_image(connection_dict, image_path):
        """"Backup the contents of source server into an image file.

        :param connection_dict: dictionary of dictionaries of connection
                                information: mysql users and host users. It can
                                have the following keys: MYSQL_SOURCE,
                                MYSQL_DEST, HOST_SOURCE and HOST_DEST.
                                Each of these keys has as a value a dict with
                                the following keys: user, hostname, port,
                                passwd and their respective values.
        :type connection_dict: dict
        :param image_path: name/path of the image that we will create with the
                           backup.
        :type image_path:  string
        """
        # Check tool requirements
        mysqldump_exe = get_tool_path(None, "mysqldump", search_path=True,
                                      required=False)
        if not mysqldump_exe:
            raise exceptions.GadgetError(
                "Could not find mysqldump executable. Make sure it is on "
                "{0}.".format(PATH_ENV_VAR))

        # Creating Server instance for source server
        source_dict = connection_dict[MYSQL_SOURCE]
        try:
            source_server = Server({'conn_info': source_dict})
        except exceptions.GadgetError as err:
            _LOGGER.error("Unable to create a Server instance for source "
                          "server. Source dict was: %s", source_dict)
            raise err

        # Connect to source server
        try:
            source_server.connect()
        except exceptions.GadgetServerError as err:
            raise exceptions.GadgetError(
                "Unable to connect to source server: {0}".format(str(err)))

        # Create config_file for mysqldump
        dump_config_file = Server.to_config_file(source_server, "mysqldump")

        # Create command list for backup
        backup_cmd = shlex.split(
            _MYSQLDUMP_IMAGE_BACKUP_CMD.format(mysqldump_exec=mysqldump_exe,
                                               config_file=dump_config_file,
                                               image_file=image_path,
                                               quote=QUOTE_CHAR))

        # enable global read_lock
        _LOGGER.debug("Locking global read lock on source server to prevent "
                      "modifications during clone.")
        source_server.toggle_global_read_lock(True)
        _LOGGER.debug("Source server locked (read-only=ON)")

        # Do the backup
        try:
            _LOGGER.debug("Dumping contents of source server to image file %s "
                          "using command: %s", image_path,
                          " ".join(backup_cmd))
            dump_process = subprocess.Popen(backup_cmd, stderr=subprocess.PIPE,
                                            universal_newlines=True)
            _, err = dump_process.communicate()
            if dump_process.returncode:
                raise exceptions.GadgetError(
                    "mysqldump exited with error code '{0}' and message: "
                    "'{1}'. ".format(dump_process.returncode,
                                     err.strip()))
            else:
                _LOGGER.info("Dumping to file was successful.")

        finally:
            # disable global read_lock
            _LOGGER.debug("Unlocking global read lock on source server.")
            source_server.toggle_global_read_lock(False)
            _LOGGER.debug("Source server unlocked. (read-only=OFF)")
            # delete created configuration file
            try:
                _LOGGER.debug("Removing configuration file '%s'",
                              dump_config_file)
                os.remove(dump_config_file)
            except OSError:
                _LOGGER.warning("Unable to remove configuration file '%s'",
                                dump_config_file)
            else:
                _LOGGER.debug("Configuration file '%s' successfully removed",
                              dump_config_file)

        _LOGGER.info("Source server contents successfully cloned to file "
                     "'%s'.", image_path)

    @staticmethod
    def move_image(connection_dict, image_path):
        """"Move an image file from source address to destination address.

        :param connection_dict: dictionary of dictionaries of connection
                                information: mysql users and host users. It can
                                have the following keys: MYSQL_SOURCE,
                                MYSQL_DEST, HOST_SOURCE and HOST_DEST.
                                Each of these keys has as a value a dict with
                                the following keys: user, hostname, port,
                                passwd and their respective values.
        :type connection_dict: dict
        :param image_path: name/path of the image that we will move from the
                           to the home folder of the destination address.
        :type image_path:  string
        """
        pass

    @staticmethod
    def restore_from_image(connection_dict, image_path):
        """"Restore an image file into the destination server.

        :param connection_dict: dictionary of dictionaries of connection
                                information: mysql users and host users. It can
                                have the following keys: MYSQL_SOURCE,
                                MYSQL_DEST, HOST_SOURCE and HOST_DEST.
                                Each of these keys has as a value a dict with
                                the following keys: user, hostname, port,
                                passwd and their respective values.
        :type connection_dict: dict
        :param image_path: name/path of the image that we will be read to do
                           the restore operation.
        :type image_path:  string
        """
        # Creating Server for destination server
        destination_dict = connection_dict[MYSQL_DEST]
        try:
            destination_server = Server({'conn_info': destination_dict})
        except exceptions.GadgetError as err:
            _LOGGER.error("Unable to create a Server instance for destination "
                          "server. Destination dict was: %s", destination_dict)
            raise err

        # Connect to destination server
        try:
            destination_server.connect()
        except exceptions.GadgetServerError as err:
            raise exceptions.GadgetError("Unable to connect to destination "
                                         "server: {0}.".format(str(err)))

        mysqlc_exe = get_tool_path(None, "mysql", search_path=True,
                                   required=False)
        if not mysqlc_exe:
            raise exceptions.GadgetError(
                "Could not find MySQL client executable. Make sure it is on "
                "{0}.".format(PATH_ENV_VAR))

        # Create config_file for mysql client
        client_config_file = Server.to_config_file(destination_server,
                                                   "client")
        # Replace image_name backslashes with forward slashes to pass it to the
        # mysql source command
        if os.name == 'nt':
            image_path = '/'.join(image_path.split(os.sep))
        # Create command list to restore the backup
        restore_cmd = shlex.split(
            _MYSQLDUMP_IMAGE_RESTORE_CMD.format(
                mysqlc_exec=mysqlc_exe, config_file=client_config_file,
                image_file=image_path, quote=QUOTE_CHAR
            ))
        try:
            _LOGGER.debug("Restoring contents of destination server from "
                          "image file %s using command: %s",
                          image_path, " ".join(restore_cmd))
            restore_process = subprocess.Popen(restore_cmd,
                                               stderr=subprocess.PIPE,
                                               universal_newlines=True)

            _, err = restore_process.communicate()
            if restore_process.returncode:
                raise exceptions.GadgetError(
                    "MySQL client exited with error code '{0}' and message: "
                    "'{1}'. ".format(restore_process.returncode, err.strip()))
            else:
                _LOGGER.info("Restoring from file was successful.")
        finally:
            # delete created configuration file
            try:
                _LOGGER.debug("Removing configuration file '%s'",
                              client_config_file)
                os.remove(client_config_file)
            except OSError:
                _LOGGER.warning("Unable to remove configuration file '%s'",
                                client_config_file)
            else:
                _LOGGER.debug("Configuration file '%s' successfully removed",
                              client_config_file)

        _LOGGER.info("Destination server contents successfully loaded from "
                     "file '%s'.", image_path)

    @staticmethod
    def pre_validation(connection_dict):
        """ Checks for requirements before clone operation is executed.

        If requirements are not met, the implementation must raise an exception
        and as a result the clone operation will be cancelled before it starts.
        The message of the exception will be logged as the cause of the clone
        operation having not met the pre-clone requirements.
        param connection_dict: dictionary of dictionaries of connection
                               information: mysql users and host users. It can
                               have the following keys: MYSQL_SOURCE,
                               MYSQL_DEST, HOST_SOURCE and HOST_DEST.
                               Each of these keys has as a value a dict with
                               the following keys: user, hostname, port,
                               passwd and their respective values.
        :raises: Exception if pre-clone requirements are not met.
        """
        # Creating Server instances for source and destination servers
        source_dict = connection_dict[MYSQL_SOURCE]
        destination_dict = connection_dict[MYSQL_DEST]
        try:
            source_server = Server({'conn_info': source_dict})
        except exceptions.GadgetError as err:
            _LOGGER.error(
                "Unable to create a Server instance for source server."
                "Source dict was: %s", source_dict)
            raise err
        try:
            destination_server = Server({'conn_info': destination_dict})
        except exceptions.GadgetError as err:
            _LOGGER.error("Unable to create a Server instance for destination "
                          "server. Destination dict was: %s", destination_dict)
            raise err

        # Connect to source and destination servers
        try:
            source_server.connect()
        except exceptions.GadgetServerError as err:
            raise exceptions.GadgetError(
                "Unable to connect to source server: {0}".format(str(err)))
        try:
            destination_server.connect()
        except exceptions.GadgetServerError as err:
            raise exceptions.GadgetError("Unable to connect to destination "
                                         "server: {0}.".format(str(err)))

        # Check if source server has the same GTID mode as destination server
        _LOGGER.debug("Checking if source server and destination have the "
                      "same GTID mode.")
        try:
            source_has_gtids = source_server.supports_gtid()
        except exceptions.GadgetServerError:
            # if GTIDs are not supported it is the same as having them disabled
            source_has_gtids = False
        try:
            destination_has_gtids = destination_server.supports_gtid()
        except exceptions.GadgetServerError:
            # if GTIDs are not supported it is the same as having them disabled
            destination_has_gtids = False

        if not source_has_gtids == destination_has_gtids:
            raise exceptions.GadgetError(
                "Cloning pre-condition check failed: Source and destination "
                "servers must have the same GTID mode.")
        if destination_has_gtids:
            # if destination has GTID support enabled, we must make sure
            # it is empty.
            gtid_executed = destination_server.get_gtid_executed()
            if gtid_executed:
                raise exceptions.GadgetError(
                    "Cloning pre-condition check failed: GTID executed set "
                    "must be empty on destination server.")

        # Check if user has super privilege on source
        # required for the set super_only
        _LOGGER.debug("Checking if MySQL source user has the required "
                      "SUPER privilege.")
        source_username = "{0}@{1}".format(source_server.user,
                                           source_server.host)
        source_user = User(source_server, source_username,
                           source_server.passwd)
        if not source_user.has_privilege('*', '*', 'SUPER'):
            raise exceptions.GadgetError(
                "SUPER privilege is required for the MySQL user '{0}' on "
                "the source server.".format(source_server.user))

        # Check if user has super privilege on destination
        _LOGGER.debug("Checking if MySQL destination user has the "
                      "required SUPER privilege.")
        dest_username = "{0}@{1}".format(destination_server.user,
                                         destination_server.host)
        dest_user = User(destination_server, dest_username,
                         destination_server.passwd)
        if not dest_user.has_privilege('*', '*', 'SUPER'):
            raise exceptions.GadgetError(
                "SUPER privilege is required for the MySQL user '{0}' on "
                "the destination server.".format(destination_server.user))

        # After the clone operation, mysql user table on destination server
        # will be replaced by the mysql user table from source server. So we
        # must make sure that:
        # *) Either source or destination users exist on source server
        #    with a hostname that matches destination server hostname.
        # If this conditions holds, then if clone operation
        # finishes successfully we are sure to be able to connect to the
        # destination server to do any post_clone verification.
        # Otherwise we must issue a warning stating that the post_clone
        # verification might fail.
        if (source_server.user_host_exists(destination_server.user,
                                           destination_server.host) or
                source_server.user_host_exists(source_server.user,
                                               destination_server.host)):
            return  # Condition holds, no need to issue a warning.
        else:
            _LOGGER.warning("Cloning will replace mysql user table on "
                            "destination server with mysql user table from "
                            "source. Since neither source user account "
                            "'%s' nor destination user account '%s' exist on "
                            "source server with a wildcard hostname (%%), the "
                            "post clone requirement check might fail because "
                            "the tool might not be able to successfully "
                            "connect to the destination server.",
                            source_server.user, destination_server.user)

    @staticmethod
    def post_validation(connection_dict):
        """ Checks for requirements after clone operation is executed.

        If requirements are not met, the implementation must raise an
        exception and as a result the clone operation will be reported as
        having failed. The message of the exception will be logged as the cause
        of the clone operation having not met post-clone requirements.
        param connection_dict: dictionary of dictionaries of connection
                               information: mysql users and host users. It can
                               have the following keys: MYSQL_SOURCE,
                               MYSQL_DEST, HOST_SOURCE and HOST_DEST.
                               Each of these keys has as a value a dict with
                               the following keys: user, hostname, port,
                               passwd and their respective values.
        :type connection_dict: dict
        :raises: Exception if post-clone requirements are not met.

        Note: This method is only executed if clone operation occurs without
        any errors.
        """
        # Creating Server instances for source and destination servers
        source_dict = connection_dict[MYSQL_SOURCE]
        destination_dict = connection_dict[MYSQL_DEST]
        try:
            source_server = Server({'conn_info': source_dict})
        except exceptions.GadgetError as err:
            _LOGGER.error(
                "Unable to create a Server instance for source server."
                "Source dict was: %s", source_dict)
            raise err
        try:
            destination_server = Server({'conn_info': destination_dict})
        except exceptions.GadgetError as err:
            _LOGGER.error("Unable to create a Server instance for destination "
                          "server. Destination dict was: %s", destination_dict)
            raise err

        # Connect to source and destination servers
        try:
            source_server.connect()
        except exceptions.GadgetServerError as err:
            raise exceptions.GadgetError(
                "Unable to connect to source server: {0}".format(str(err)))
        try:
            # try to connect with original destination user credentials
            destination_server.connect()
        except exceptions.GadgetServerError as dest_err:
            # if failed, try to use source user credentials
            orig_dest_user = destination_dict["user"]
            destination_dict["user"] = source_dict["user"]
            destination_dict["passwd"] = source_dict["passwd"]
            try:
                destination_server = Server({'conn_info': destination_dict})
            except exceptions.GadgetError as e:
                _LOGGER.error(
                    "Unable to create a Server instance for destination "
                    "server. Destination dict was: %s", destination_dict)
                raise e
            try:
                destination_server.connect()
            except exceptions.GadgetServerError as source_err:
                raise exceptions.GadgetError(
                    "Unable to connect to destination server after using both "
                    "destination user '{0}' and source user '{1}' "
                    "credentials. Error for destination user: '{2}'. Error "
                    "for source user: '{3}'."
                    "".format(orig_dest_user, source_server.user,
                              str(dest_err), str(source_err)))

        # if GTIDs are enabled we must check that GTID executed set is the same
        # for both servers.
        try:
            source_gtid_executed = source_server.get_gtid_executed(
                skip_gtid_check=False)
        except exceptions.GadgetError:
            source_gtid_executed = ""  # if GTIDs are disabled assume empty set
        try:
            dest_gtid_executed = destination_server.get_gtid_executed(
                skip_gtid_check=False)
        except exceptions.GadgetError:
            dest_gtid_executed = ""  # if GTIDs are disabled assume empty set
        if not source_gtid_executed == dest_gtid_executed:
            raise exceptions.GadgetError(
                "Cloning post-condition check failed. Source and destination "
                "servers don't have the same GTID_EXECUTED value."
            )
