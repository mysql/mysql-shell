# Copyright (c) 2018, 2024, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

include_directories(BEFORE
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/mysqlshdk/include"
  "${CMAKE_SOURCE_DIR}/mysql-secret-store/include"
  "${CMAKE_SOURCE_DIR}/mysql-secret-store/core")

set(core_SRC
  argument_parser.cc
  erase_command.cc
  get_command.cc
  helper.cc
  json_converter.cc
  list_command.cc
  main.cc
  program.cc
  store_command.cc
  version_command.cc
  ${CMAKE_SOURCE_DIR}/mysqlshdk/shellcore/interrupt_helper.cc
)

add_convenience_library(mysql-secret-store-core ${core_SRC})

target_link_libraries(mysql-secret-store-core
  mysql-secret-store-api
)
