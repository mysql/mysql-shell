#  Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License, version 2.0,
#  as published by the Free Software Foundation.
#
#  This program is also distributed with certain software (including
#  but not limited to OpenSSL) that is licensed under separate terms, as
#  designated in a particular file or component or in included license
#  documentation.  The authors of MySQL hereby grant you an additional
#  permission to link the program and your derivative works with the
#  separately licensed software that they have included with MySQL.
#  This program is distributed in the hope that it will be useful,  but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
#  the GNU General Public License, version 2.0, for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import sys

sys.path.append("lib")

import unittest
import utils

import os

test_file_pattern = "*_t"

for arg in sys.argv[1:]:
    if arg.startswith("--record="):
        _, _, testname = arg.partition("=")

        utils.tests_to_record.append(testname)
    elif not arg.startswith("-"):
        if arg.endswith("_t.py"):
            arg = arg[:5]
        test_file_pattern = arg


os.putenv("TMPDIR", "/tmp")


tests = unittest.TestLoader().discover("t", pattern=test_file_pattern+".py")

suite = unittest.TestSuite(tests)

unittest.TextTestRunner(verbosity=2).run(suite)

