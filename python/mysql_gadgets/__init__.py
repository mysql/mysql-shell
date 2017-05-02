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

"""mysql gadgets library"""

import inspect
import os
import sys

from mysql_gadgets.exceptions import GadgetError, GadgetVersionError

# Major, Minor, Patch, Status
VERSION = (2, 1, 0, '', 0)
# Future versions will have to include only the X, Y (no Z).

VERSION_STRING = "%s.%s.%s" % VERSION[0:3]

COPYRIGHT_YEAR = "2016, 2017, "

COPYRIGHT_FULL = (
    "Copyright (c) %s Oracle and/or its affiliates. All rights reserved.\n\n"
    "Oracle is a registered trademark of Oracle Corporation and/or its "
    "affiliates. Other names may be trademarks of their respective owners."
    % COPYRIGHT_YEAR)

LICENSE = "GPLv2"

VERSION_FRM = "{program} version %s" % VERSION_STRING

LICENSE_FRM = (VERSION_FRM + "\n" + COPYRIGHT_FULL)
PYTHON_MIN_VERSION = (2, 7, 0)
PYTHON_MAX_VERSION = (4, 0, 0)
CONNECTOR_MIN_VERSION = (1, 2, 1)

# Using mathematical notation for intervals, supported MYSQL versions are as
# follows: [MIN_MYSQL_VERSION, MAX_MYSQL_VERSION [
MIN_MYSQL_VERSION = (5, 7, 17)  # minimum required version (supported)
MAX_MYSQL_VERSION = (9, )       # maximum mysql version (not supported)


def check_expected_version(expected_version):
    """ Check the expected version of the used tools.

    Compare the given version with the current version of the modules/tools
    raising an exception if it is not compatible.

    For the expected version to be considered compatible with the current
    version, the major version number must be the same and the minor version
    number of the current version must be greater or equal than the expected
    version. The patch version number is ignored.

    Rationale: The major version number shall always be incremented if an
    incompatible change is made. The minor version number shall be
    incremented if new features are added in a backward-compatible manner,
    meaning that previous features are expected to continue to work (but the
    new added feature is only expected to be available starting from that
    minor version). The patch version is incremented for backward-compatible
    changes (e.g. bug fixes).

    :param expected_version: Excepted version to compare to the current one.
    :type expected_version:  String

    :raises GadgetError: If the specified expected version value/format is
                         invalid.
    :raises GadgetVersionError: If the expected version is not compatible with
                                the current one.
    """
    # Validate expected_version value and format.
    if expected_version:
        version_values = expected_version.split('.')
        if len(version_values) > 3:
            raise GadgetError("Invalid expected version value: '{0}'. Please "
                              "specify at most 3 version number parts "
                              "(format: MAJOR[.MINOR[.PATCH]])."
                              "".format(expected_version))
    else:
        raise GadgetError("Invalid expected version value: '{0}'. Please "
                          "specify a valid version string (format: "
                          "MAJOR[.MINOR[.PATCH]]).".format(expected_version))

    # Get major, minor and patch version numbers.
    version_num = [-1, -1, -1]
    for idx, v_num in enumerate(version_values):
        if idx == 0:
            v_num_type = 'major'
        elif idx == 1:
            v_num_type = 'minor'
        else:
            v_num_type = 'patch'
        try:
            version_num[idx] = int(v_num)
            if version_num[idx] < 0:
                raise GadgetError("Invalid integer for the expected {0} "
                                  "version, it cannot be a negative number: "
                                  "'{1}'.".format(v_num_type, v_num))
        except ValueError:
            raise GadgetError("Invalid integer for the expected {0} version "
                              "number: '{1}'.".format(v_num_type, v_num))

    # Validate the expected version comparing it to the current version.
    if (version_num[0] != VERSION[0]) or (version_num[1] > VERSION[1]):
        # The major expected version number must be the same as the one of the
        # current tool version.
        # The minor expected version number must be lower or equal than the one
        # of the current tool version.
        # The patch expected version number is ignored since it is associated
        # to compatible changes.
        raise GadgetVersionError(
            "The expected version is not compatible with the current version. "
            "Current version '{0}' and expected version: '{1}'."
            "".format('.'.join(map(str, VERSION[0:3])), expected_version))


def check_python_version(min_version=PYTHON_MIN_VERSION,
                         max_version=PYTHON_MAX_VERSION,
                         raise_exception_on_fail=False, name=None,
                         exit_on_fail=True, return_error_msg=False,
                         json_msg=False):
    """Check the Python version compatibility.

    By default this method uses constants to define the minimum and maximum
    Python versions required. It's possible to override this by passing new
    values on ``min_version`` and ``max_version`` parameters.
    It will run a ``sys.exit`` or raise a ``GadgetError`` if the version of
    Python detected it not compatible.

    :param min_version:             Tuple with the minimum Python version
                                    required (inclusive).
    :type min_version:              tuple
    :param max_version:             Tuple with the maximum Python version
                                    required (exclusive).
    :type max_version:              tuple
    :param raise_exception_on_fail: It will raise a ``GadgetError`` if
                                    True and Python detected is not compatible.
    :type raise_exception_on_fail:  boolean
    :param name:                    Custom name for the script, if not provided
                                    it will get the module name from where this
                                    function was called.
    :type name:                     string
    :param exit_on_fail:            If True, issue exit() else do not exit()
                                    on failure.
    :type exit_on_fail:             boolean
    :param return_error_msg:        If True, and is not compatible
                                    returns (result, error_msg) tuple.
    :type return_error_msg:         boolean
    :param json_msg:                If true return the error message in JSON
                                    format '{"type": "ERROR", "msg": "..."}'
                                    when exit_on_fail=True.
                                    By default: False, JSON format not used.
    :type json_msg:                 boolean

    :return:   True if Python version is compatible, otherwise False if
               'raise_exception_on_fail' is set to False or a tuple
               (False, error message) if 'return_error_msg' is set to True.
    :rtype:    boolean or tuple

    :raises GadgetVersionError: if the detected Python version is not
                                compatible and 'raise_exception_on_fail'
                                is set to True.
    """

    # Only use the fields: major, minor and micro
    sys_version = sys.version_info[:3]

    # Test min version compatibility
    is_compat = min_version <= sys_version

    # Test max version compatibility if it's defined
    if is_compat and max_version:
        is_compat = sys_version < max_version

    if not is_compat:
        if not name:
            # Get the utility name by finding the module
            # name from where this function was called
            frm = inspect.stack()[1]
            mod = inspect.getmodule(frm[0])
            mod_name = os.path.splitext(
                os.path.basename(mod.__file__))[0]
            name = '%s gadget' % mod_name

        # Build the error message
        if max_version:
            max_version_error_msg = 'or higher and lower than %s' % \
                '.'.join([str(el) for el in max_version])
        else:
            max_version_error_msg = 'or higher'

        error_msg = (
            'Python version %(min_version)s %(max_version_error_msg)s is '
            'required. The version of Python detected was '
            '%(sys_version)s. You may need to install or redirect the '
            'execution to an environment that includes a compatible Python '
            'version.'
        ) % {
            'sys_version': '.'.join([str(el) for el in sys_version]),
            'min_version': '.'.join([str(el) for el in min_version]),
            'max_version_error_msg': max_version_error_msg
        }

        if raise_exception_on_fail:
            raise GadgetVersionError(error_msg)

        if exit_on_fail:
            if json_msg:
                sys.exit('{"type": "ERROR", "msg": "%s"}\n' % error_msg)
            else:
                sys.exit("ERROR: %s\n" % error_msg)

        if return_error_msg:
            return is_compat, error_msg

    return is_compat


def check_connector_python(min_version=CONNECTOR_MIN_VERSION, json_msg=False):
    """Check connector/python version.

    Check to see if Connector/Python is installed, accessible and
    meets the minimum required version.

    By default this method uses constants to define the minimum
    C/Python version required. It's possible to override this by passing a new
    value to ``min_version`` parameter.

    This method terminates the execution of the application 'sys.exit()' if
    connector/python is not found or it does not meet the minimum version
    requirement.

    :param min_version: Tuple with the minimum C/Python version required
        (inclusive).
    :type min_version:  tuple
    :param json_msg:    If true return the error message in JSON format
                        '{"type": "ERROR", "msg": "..."}'.
                        By default: False, JSON format not used.
    :type json_msg:     boolean
    """
    is_compatible = True
    try:
        import mysql.connector  # pylint: disable=W0612
    except ImportError:
        error_msg = (
            "The MySQL Connector/Python module was not found and "
            "it is required to be installed. Please check your paths or "
            "download and install the Connector/Python from "
            "http://dev.mysql.com.")
        if json_msg:
            sys.exit('{"type": "ERROR", "msg": "%s"}\n' % error_msg)
        else:
            sys.exit("ERROR: %s\n" % error_msg)
    else:
        try:
            sys_version = mysql.connector.version.VERSION[:3]
        except AttributeError:
            is_compatible = False

    if not is_compatible or sys_version < min_version:
        error_msg = (
            "The MySQL Connector/Python module was found "
            "but it is either not properly installed or it is "
            "an old version. Connector/Python version > '{0}' is "
            "required. Download and install Connector/Python from "
            "http://dev.mysql.com.").format(min_version)
        if json_msg:
            sys.exit('{"type": "ERROR", "msg": "%s"}\n' % error_msg)
        else:
            sys.exit("ERROR: %s\n" % error_msg)
