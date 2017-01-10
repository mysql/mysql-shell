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

"""Package where adapter modules should be placed"""

import abc
import pkgutil


HOST_SOURCE = "HOST_SOURCE"
HOST_DEST = "HOST_DEST"
MYSQL_SOURCE = "MYSQL_SOURCE"
MYSQL_DEST = "MYSQL_DEST"


def _with_metaclass(mcls):
    """Decorator to allow use of metaclass compatible with Python2 and Python3.

    @param mcls: type of the metaclass to apply to the class being decorated
    @type mcls: metaclass
    """
    def decorator(cls):
        """function that wraps the class setting its metaclass type to the
        type received as argument.
        """
        body = vars(cls).copy()
        # clean out class body
        body.pop('__dict__', None)
        body.pop('__weakref__', None)
        return mcls(cls.__name__, cls.__bases__, body)
    return decorator


class _abstractstatic(staticmethod):
    """"Decorator Class to allow that implements @abstractmethod
    @staticmethod into a single decorator. Pyhon 3.3 supports using both, but
    python 2.7 doesn't."""
    __slots__ = ()

    __isabstractmethod__ = True

    def __init__(self, function):
        # Set the __isabstractmethod__ to True which abstractmethod
        # does. See https://www.python.org/dev/peps/pep-3119/
        function.__isabstractmethod__ = True
        # Apply staticmethod constructor on the function being
        # decorated.
        super(_abstractstatic, self).__init__(function)


@_with_metaclass(abc.ABCMeta)
class StreamClone(object):
    """Class that defines the API for cloning via stream.
    """
    # pylint: disable=W0613
    @_abstractstatic
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
        return


@_with_metaclass(abc.ABCMeta)
class ImageClone(object):
    """Class that defines the API for cloning via image files.
    """
    # pylint: disable=W0613
    @_abstractstatic
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
        return

    @_abstractstatic
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
        return

    @_abstractstatic
    def restore_from_image(connection_dict, image_path):
        """Restore an image file into the destination server.

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
        return


@_with_metaclass(abc.ABCMeta)
class WithValidation(object):
    """Class that defines the API for checking pre and post clone requirements.

   Instances of this class will run the pre-validation method before
   attempting the clone operation and the post_validation method after the
   clone operation.
   If the pre_validation method fails (raises Exception) then the clone
   operation stops and nothing is done.
   If the post_validation method fails (raises Exception) then the script
   will exit with an error code.
   """

    # pylint: disable=W0613
    @_abstractstatic
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
        return

    @_abstractstatic
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
        return


# Load all modules in this package
__all__ = []
for _, name, _ in pkgutil.walk_packages(__path__):
    __all__.append(name)
