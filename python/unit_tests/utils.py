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
"""
Utility module for tests to extend native unittest and allow parameters to be
passed through the command line (e.g., specify a server to connect to).
"""

import difflib
import random
import unittest

from abc import ABCMeta, abstractproperty

from mysql_gadgets.exceptions import GadgetError

from mysql_gadgets.common.group_replication import (check_server_requirements,
                                                    get_req_dict,)
from mysql_gadgets.common.tools import is_listening

# Options (keys) passed to the GadgetsTestCase class from script to run tests.
SERVER_CNX_OPT = 'servers_cnx'


def get_file_differences(file_a, file_b):
    """Compare two files and return their differences.

    Compares the contents of both files and returns an unified diff format
    string with the differences. If there are no differences returns an empty
    string.

    :param file_a: absolute path to one of the files to be compared
    :type file_a: str
    :param file_b: absolute path to one of the files to be compared
    :type file_b: str

    :return: unified diff string with the differences if there are differences
             and if there are no differences return an empty string.
    :rtype: str
    :raises GadgetError: if an error happens while opening the provided files.
    """
    # Check that backup file is equal to original file
    try:
        with open(file_a, 'U') as fa:
            with open(file_b, 'U') as fb:
                diff = list(difflib.unified_diff(
                    fa.readlines(), fb.readlines(),
                    fromfile=file_a, tofile=file_b))
                return "\n".join(diff)

    except (IOError, OSError) as err:
        # wrap error messages in a GadgetError
        raise GadgetError(str(err))


def skip_if_not_GR_approved(server):
    """Verify GR basic requirements on the given server.

    :@param param: server to verify.
    :type param: Server instance.

    :raise SkipTest: if the server does not fulfill the requirements for GR.
    """
    try:
        req_dict = get_req_dict(server, None, None, None)
        check_server_requirements(server, req_dict, None,
                                  verbose=False, dry_run=False,
                                  skip_schema_checks=False, update=False,
                                  skip_backup=True)
    except:
        raise unittest.SkipTest("Provided server must fulfill the GR "
                                "plugin requirements.")


class GadgetsTestCase(unittest.TestCase):
    """Test case class that defines some convenience methods for MySQL
    Gadget test cases.
    """
    __metaclass__ = ABCMeta

    def __init__(self, testname, options=None):
        super(GadgetsTestCase, self).__init__(testname)
        # Options passed to the test as parameter from script to run the tests.
        self.options = options

    @abstractproperty
    def num_servers_required(self):
        """Abstract property with the number of servers required by the test.
        This property needs to be defined by sub classes.
        """
        pass


def get_free_random_port(low_limit=3000, upper_limit=6553):
    """Generate a random local port not currently in use.

    @return: A local port not in use.
    @rtype: int
    """

    # Use a port not already in use.
    test_port = 3306
    tries = 0
    port_found = False
    while tries < 10 and not port_found:
        tries += 1
        if not is_listening("localhost", test_port):
            port_found = True
        else:
            test_port = random.randint(low_limit, upper_limit)

    return test_port
