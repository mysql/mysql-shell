# Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

MACRO(SETUP_TESTING)
    include(CTest)
    include(gtest)

    # Target to update the test case file
    # Call 'make testgroups' to update the TestGroups.txt file after adding a new test case.
    add_custom_target(testgroups
            ${CMAKE_BINARY_DIR}/unittest/run_unit_tests --generate_test_groups=${CMAKE_SOURCE_DIR}/unittest/TestGroups.txt
            )

    IF(WITH_COVERAGE)
        MESSAGE(STATUS "Enabling coverage support for gcc")

        #ADD_DEFINITIONS("-g -O0 -Wall -W -Wshadow -Wunused-variable -Wunused-parameter -Wunused-function -Wunused -Wno-system-headers -Wno-deprecated -Woverloaded-virtual -Wwrite-strings -fprofile-arcs -ftest-coverage")
        ADD_DEFINITIONS("-g -O0 -fprofile-arcs -ftest-coverage")

        SET(GCOV_LDFLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
    ENDIF(WITH_COVERAGE)

ENDMACRO(SETUP_TESTING)
