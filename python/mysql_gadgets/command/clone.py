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

"""This file contains the functionality to clone a server
"""

import logging
import importlib
import os

from mysql_gadgets.common import server, tools
from mysql_gadgets.common.logger import CustomLevelLogger
from mysql_gadgets import exceptions
from mysql_gadgets import adapters

# get module logger
logging.setLoggerClass(CustomLevelLogger)
_LOGGER = logging.getLogger(__name__)


def clone_server(connection_dict, adapter_name, image_name=None):
    """Clone server contents from source to destination.

    :param connection_dict: dictionary of dictionaries of connection
                            information: mysql users and host users. It can
                            have the following keys: source_mysql,
                            destination_mysql, source_host, destination_host.
                            Each of these keys has as a value a dict with the
                            following keys: user, hostname, port, passwd and
                            their respective values.
    :type connection_dict: dict
    :param adapter_name: name of the adapter to be used
    :type adapter_name: str
    :param image_name: Name of the image file to be used. If a name is
                       provided cloning via image will be used. However, if
                       its value is None then stream cloning will be used.
    :type image_name: str
    """

    # Create source and destination connection dictionaries
    source_dict = connection_dict[adapters.MYSQL_SOURCE]

    destination_dict = connection_dict[adapters.MYSQL_DEST]

    # Creating Server instances for source and destination servers
    try:
        source = server.Server({'conn_info': source_dict})
    except exceptions.GadgetError as err:
        _LOGGER.error("Unable to create a Server instance for source server."
                      "Source dict was: %s", source_dict)
        raise err
    try:
        destination = server.Server({'conn_info': destination_dict})
    except exceptions.GadgetError as err:
        _LOGGER.error("Unable to create a Server instance for destination "
                      "server. Destination dict was: %s", destination_dict)
        raise err

    # Connect to source and destination servers
    try:
        source.connect()
    except exceptions.GadgetServerError as err:
        raise exceptions.GadgetError("Unable to connect to source server: {0}"
                                     "".format(str(err)))
    try:
        destination.connect()
    except exceptions.GadgetServerError as err:
        raise exceptions.GadgetError("Unable to connect to destination "
                                     "server: {0}.".format(str(err)))

    # we cannot use the same server as both source and destination
    _LOGGER.debug("Checking if source and destination are the same server")
    same_server = server.check_hostname_alias(source_dict, destination_dict)
    if same_server:
        raise exceptions.GadgetError(
            "The source and destination servers cannot be the same server.")
    else:
        _LOGGER.debug("Source and destination are not the same server.")

    # import all adapter modules
    for module in adapters.__all__:
        # pylint: disable=E1101
        _LOGGER.debug("Importing module %s.%s", adapters.__package__, module)
        importlib.import_module("{0}.{1}".format(adapters.__package__, module))

    # Get list of adapters with Validation and check if specified adapter
    # is one of them
    adapters_with_validation = tools.get_subclass_dict(adapters.WithValidation)
    validation_adapter = adapters_with_validation.get(adapter_name, None)
    if validation_adapter is None:
        _LOGGER.info("Adapter'%s' doesn't exist or contains no pre-clone and "
                     "post-clone requirement validations to be done.",
                     adapter_name)
    else:
        _LOGGER.info("Checking pre-clone requirements of adapter '%s'.",
                     adapter_name)
        # This method might raise an exception that will be caught by the
        # mysql-server-clone script
        validation_adapter.pre_validation(connection_dict)
        _LOGGER.info("Pre-clone requirement validation passed")
    # Check if we are using stream or image type of backup
    if image_name is None:
        # Using image stream backup, so we need to check if the adapter
        # specified supports it
        file_adapters = tools.get_subclass_dict(adapters.StreamClone)
        adapter = file_adapters.get(adapter_name, None)
        if adapter is None:
            raise exceptions.GadgetError(
                "There is no stream cloning supporting adapter named '{0}'."
                " Use one of the following values for the --using option: "
                "{1}.".format(adapter_name, ", ".join(file_adapters.keys())))

        # pylint: disable=E1101
        _LOGGER.step("Dumping the contents of source server into destination "
                     "server via streaming using adapter '%s'.", adapter_name)
        adapter.clone_stream(connection_dict)
    else:
        image_adapters = tools.get_subclass_dict(adapters.ImageClone)
        adapter = image_adapters.get(adapter_name, None)
        if adapter is None:
            raise exceptions.GadgetError(
                "There is no image cloning supporting adapter named '{0}'. "
                " Use one of the following values for the --using option: "
                "{1}.".format(adapter_name, ", ".join(image_adapters.keys())))

        # Check if image file is absolute path or not
        if os.path.isabs(image_name):
            image_path = image_name
        else:
            # if relative path or filename
            image_path = os.path.abspath(image_name)

        # Raise exception if file already exists
        if os.path.isfile(image_path):
            raise exceptions.GadgetError(
                "Cannot create image file at {0}. File already exists."
                "".format(image_path))

        # pylint: disable=E1101
        _LOGGER.step("Dumping the contents of source server into destination "
                     "server via image '%s' using adapter '%s'. ", image_path,
                     adapter_name)
        adapter.backup_to_image(connection_dict, image_path)
        adapter.move_image(connection_dict, image_path)
        adapter.restore_from_image(connection_dict, image_path)

    if validation_adapter is not None:
        _LOGGER.info("Checking post-clone requirements of adapter '%s'.",
                     adapter_name)
        # This method might raise an exception that will be caught by the
        # mysql-server-clone script
        validation_adapter.post_validation(connection_dict)
        _LOGGER.info("Post-clone requirement validation passed")
