#! /usr/bin/python
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
"""Script to run the gadgets unit tests"""

import sys
import traceback

from unittest import (
    TestLoader,
    TestSuite,
    TestCase,
    TextTestRunner,
)

import unit_tests
from unit_tests.utils import SERVER_CNX_OPT

from mysql_gadgets import check_connector_python
from mysql_gadgets.common import options

_UNIT_TEST_PKG = 'unit_tests.{0}'


def check_option_errors(parser_obj, args_obj):
    """Check options for errors.

    :param parser_obj:  Option parser instance.
    :param args_obj:    Args object returned by parse_args().
    """
    # Check errors for --skip-tests option.
    if args_obj.skip_tests == '':
        parser_obj.error('Option --skip-tests requires a non-empty value.')
    if args_obj.skip_tests:
        non_empty_list = [
            e for e in args_obj.skip_tests.split(',') if e.strip()
        ]
        if len(non_empty_list) == 0:
            parser_obj.error("Option --skip-tests requires a comma-separated "
                             "list of non-empty values. Empty list of values "
                             "specified: '{0}'.".format(args_obj.skip_tests))


def skip_tests(skip_tests_option, tests_to_run):
    """Skip specified tests.

    :param skip_tests_option: Value for the --skip-tests option.
    :param tests_to_run:      Set of specific tests to execute.

    :return: Set of tests to run excluding those specified by the --skip-tests
             option.
    """
    if skip_tests_option:
        tests_not_found = set()
        tests_to_skip = set(
            [t.strip() for t in skip_tests_option.split(',') if t.strip()]
        )
        for test in tests_to_skip:
            if test in tests_to_run:
                tests_to_run.remove(test)
            else:
                tests_not_found.add(test)
        # Warn user about invalid tests for the --skip-tests option.
        if len(tests_not_found):
            print('\nWARNING: Some specified tests to skip do not match any '
                  'test to run: {0}.\n'.format(','.join(tests_not_found)))

    return tests_to_run

if __name__ == '__main__':
    # Check connector/python required version.
    check_connector_python()

    # Create argument parser.
    parser = options.GadgetsArgumentParser()

    parser.add_argument('tests', metavar='TEST_NAME', type=str, nargs='*',
                        help='Unit test to execute. If none are specified '
                             'then all unit tests will be executed.')

    # Add version.
    options.add_version_option(parser)

    # Add verbose option.
    options.add_verbose_option(parser)

    # Add failfast option.
    parser.add_argument("--failfast", "-f", action="store_true",
                        dest="failfast",
                        help="Stop tests execution on first failure")

    # Add server.
    options.add_append_connection_option(parser, ["-s", "--server"], "server",
                                         required=False)

    # Add option to read passwords from stdin.
    options.add_stdin_password_option(parser)

    parser.add_argument("--skip-tests", "-i", action="store",
                        dest="skip_tests", type=str, default=None,
                        help="Comma-separated list of tests to skip. "
                             "Example: test_server,test_tools")

    # Parse arguments.
    args = parser.parse_args()

    # Check errors for specified options.
    check_option_errors(parser, args)

    # ask for passwords
    options.read_passwords(args)

    # Set verbosity level based on --verbose option.
    verbosity = 3 if args.verbose else 1

    # Set options dictionary to use by required tests.
    test_options = {
        SERVER_CNX_OPT: args.server,
    }

    # Set tests to execute.
    tests = args.tests if args.tests else unit_tests.__all__

    # Skip specified tests, i.e., remove from args
    tests = skip_tests(args.skip_tests, set(tests))

    # Number of server options required.
    num_srv_needed = 0

    # Load tests.
    testloader = TestLoader()
    suite = TestSuite()
    try:
        for test_name in tests:
            # Dynamically import test module
            module = __import__(_UNIT_TEST_PKG.format(test_name))
            test_module = getattr(module, test_name)
            # Iterate over the list of attributes for test module to find valid
            # TestCase objects.
            for name in dir(test_module):
                # Process object if subclass of TestCase.
                obj = getattr(test_module, name)
                if isinstance(obj, type) and issubclass(obj, TestCase):
                    # Check if object is subclass tests.utils.TestCase to pass
                    # required arguments.
                    use_args = True \
                        if issubclass(obj, unit_tests.utils.GadgetsTestCase) \
                        else False
                    # Get test names (methods).
                    testnames = testloader.getTestCaseNames(obj)
                    for testname in testnames:
                        if use_args:
                            # Add test with specific arguments.
                            test_instance = obj(testname, options=test_options)
                            suite.addTest(test_instance)
                            # Store max number of servers required by test.
                            num_servers = test_instance.num_servers_required
                            if num_servers > num_srv_needed:
                                num_srv_needed = num_servers
                        else:
                            # Add test without arguments (default).
                            suite.addTest(obj(testname))
    except Exception as err:  # pylint: disable=W0703
        sys.stderr.write("ERROR: Could not load tests to run: {0}"
                         "\n".format(err))
        if args.verbose:
            sys.stderr.write("DEBUG:\n")
            traceback.print_exception(*sys.exc_info())
        sys.exit(1)

    # Check if some options are required for some tests (e.g., --server).
    num_used_srv_opt = 0 if args.server is None else len(args.server)
    if num_srv_needed > num_used_srv_opt:
        parser.error("{0} server(s) required to run the tests but {1} "
                     "specified. Use the --server option to specify all "
                     "needed servers.".format(num_srv_needed,
                                              num_used_srv_opt))
    if num_used_srv_opt > num_srv_needed:
        print("WARNING: {0} server(s) required to run the tests and {1} "
              "specified with the --server option. The last {2} specified "
              "server(s) will be ignored."
              "".format(num_srv_needed, num_used_srv_opt,
                        num_used_srv_opt - num_srv_needed))

    # Redirect test output to stdout for a better merge with
    # "print" style temporary debug statements.
    # Follow verbosity and failfast options.
    test_runner = TextTestRunner(stream=sys.stdout,
                                 verbosity=verbosity,
                                 failfast=args.failfast,
                                 buffer=True)
    result = test_runner.run(suite)

    # Return with proper exit code depending on the tests results.
    if not result.wasSuccessful():
        sys.exit(1)
    else:
        sys.exit(0)
