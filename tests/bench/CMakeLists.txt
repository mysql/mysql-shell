# Copyright (c) 2022, Oracle and/or its affiliates. All rights reserved.
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

if (NOT WITH_TESTS_BENCHMARK)
  return()
endif()

add_shell_executable(bench_json_reader json_reader.cc TRUE)
if (WITH_XRAY)
  target_compile_options(bench_json_reader PRIVATE -fxray-instrument -fxray-instruction-threshold=32)
endif()

TARGET_INCLUDE_DIRECTORIES(bench_json_reader PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/mysqlshdk/include "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")
target_link_libraries(bench_json_reader mysqlshdk-static api_modules)

