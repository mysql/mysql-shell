# Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA


# cmake -DHEADERS_DIR=... -DLIBS_DIR=.. -DMYSQL_CLIENT_LIB=... ..

file(GLOB lib_SRC
      "simple_shell_client.cc"
      "simple_shell_client.h"
      "boost_code.cc"
)

add_library(shell_client_lib SHARED ${lib_SRC})

include_directories(${HEADERS_DIR})

if(WIN32)
  find_library(lib_mysqlsh "mysqlsh" "${LIBS_DIR}")
  find_library(lib_mysqlshmods "mysqlshmods" "${LIBS_DIR}")
  find_library(lib_mysqlshtypes "mysqlshtypes" "${LIBS_DIR}")
  find_library(lib_mysqlxtest "mysqlxtest" "${LIBS_DIR}")
  find_library(lib_mysqlxtest_pb "mysqlxtest_pb" "${LIBS_DIR}")
  find_library(lib_v8 "v8" "${LIBS_DIR}")

  target_link_libraries(shell_client_lib
    ${lib_mysqlsh}
    ${lib_mysqlshmods}
    ${lib_mysqlshtypes}
    ${lib_mysqlxtest}
    ${lib_mysqlxtest_pb}
    ${lib_v8}
    "${MYSQL_CLIENT_LIB}")
else()
  find_library(lib_mysqlsh "mysqlsh" "${LIBS_DIR}")
  find_library(lib_mysqlshmods "mysqlshmods" "${LIBS_DIR}")
  find_library(lib_mysqlxtest "mysqlxtest" "${LIBS_DIR}")
  find_library(lib_v8 "v8" "${LIBS_DIR}")

  target_link_libraries(shell_client_lib
    ${lib_mysqlsh}
    ${lib_mysqlshmods}
    ${lib_mysqlxtest}
    ${lib_v8}
    "${MYSQL_CLIENT_LIB}")
endif()

add_definitions(-DSHELL_CLIENT_NATIVE_EXPORTS)
add_definitions(-DBOOST_ALL_NO_LIB)

