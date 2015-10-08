#  Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; version 2 of the
#  License.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
#  02110-1301  USA

import unittest
import subprocess
import os
import StringIO
import sys
import select
import inspect
import difflib

# find mysqlx
mysqlx_path = "mysqlx"

tests_to_record = []






if os.getenv("MYSQLX_SHELL_BINDIR"):
    mysqlx_path=os.path.join(os.getenv("MYSQLX_SHELL_BINDIR"), "mysqlx")
else:
    mysqlx_path="../mysqlx"

if not os.path.exists(mysqlx_path):
    print "mysqlx executable not found, please run it as shell-test/mxtr from the build directory"
    sys.exit(1)


def process_args(argl):
    tmpdir = os.getenv("TMPDIR")
    l = []
    for i in argl:
        l.append(i.replace("$TMPDIR", tmpdir))
    return l


def mysqlx(*argv):
    p = subprocess.Popen([mysqlx_path]+process_args(argv), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out, err = p.communicate()
    sys.stdout.write(out)
    return p.returncode


def mysqlx_capture(*argv):
    p = subprocess.Popen([mysqlx_path]+process_args(argv), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out, err = p.communicate()
    sys.stdout.write(out)
    return p.returncode, out


def mysqlx_with_stdin(stdin, *argv):
    p = subprocess.Popen([mysqlx_path]+process_args(argv), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, stdin=subprocess.PIPE)
    if type(stdin) is list:
        out, err = "", ""
        for line in stdin:
            p.stdin.write(line+"\n")
            # check if stdout readable
            while True:
                r, w, x = select.select([p.stdout.fileno()], [], [], 0.5)
                if r:
                    out += p.stdout.read(1)
                else:
                    break
        outl, errl = p.communicate()
        out += outl
    else:
        out, err = p.communicate(stdin)
    sys.stdout.write(out)
    return p.returncode, out


def checkstdout(fun):
    """Decorator to be used when a test should have its output compared to
    output from a previously recorded run.

    @checkoutput
    def test_this(self):
        print 1+1
    """
    def decorated(*args):
        output = StringIO.StringIO()
        real_stdout = sys.stdout
        sys.stdout = output

        r = fun(*args)

        sys.stdout = real_stdout

        actual = output.getvalue()


        # find the file where the test is located
        test_dir, test_file = os.path.split(inspect.getfile(fun))
        test_name = fun.__name__
        full_test_name = os.path.splitext(test_file)[0]+"@"+test_name
        expected_filename = os.path.join(test_dir[:-2], "r", full_test_name+".result")

        actual_filename = os.path.join(os.getenv("TMPDIR"), os.path.split(expected_filename)[1])

        f = open(actual_filename, "w+")
        f.write(actual)
        f.close()

        if full_test_name in tests_to_record:
            print "Recording", expected_filename
            f = open(expected_filename, "w+")
            f.write(actual)
            f.close()
        else:
            if os.path.exists(expected_filename):
                expected = open(expected_filename).read()
                diffs = difflib.unified_diff(expected.split("\n"), actual.split("\n"), expected_filename, actual_filename, lineterm="")
                text = []
                for line in diffs:
                    text.append(line)
                if text:
                    print "Test output file mismatch for %s." % test_file
                    print "Result (%s) does not match expected output from %s." % (actual_filename, expected_filename)
                    print "\n".join(text)
                    raise Exception("@checkstdout output mismatch")
            else:
                print "Result file %s does not exist." % expected_filename
                print "=====BEGIN OUTPUT====="
                print actual
                print "=====END OUTPUT====="
                print "Use --record=%s to create it." % full_test_name

        return r

    return decorated


class ShellTestCase(unittest.TestCase):

    def setUp(self):
        self.tmpfiles = []

    def tearDown(self):
        for f in self.tmpfiles:
            os.unlink(f)


    def tmpfile(self, name):
        path = os.path.join(os.getenv("TMPDIR"), name)
        f = open(path, "w+")
        self.tmpfiles.append(path)
        return f
