# Copyright (c) 2020, Oracle and/or its affiliates.
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

function(add_fuzz_test fuzz_target)
  message(STATUS "target=${fuzz_target}, srcs=${ARGN}")

  add_shell_executable("${fuzz_target}" "${ARGN}" TRUE)

  TARGET_INCLUDE_DIRECTORIES("${fuzz_target}" PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/mysqlshdk/include
    ${PROJECT_SOURCE_DIR}/mysqlshdk/libs
  )
  # Instead of compiling all *.cc files listed in fuzz_uri_parser_SRC we can
  # link libraries, but then we lose information about code coverage in
  # uri_parser.cc
  # target_link_libraries(fuzz_uri_parser db shellcore-minimal)
  LIBFUZZER_ADD_TEST(fuzz_uri_parser
    INITIAL_CORPUS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../corpus/fuzz_uri_parser)
endfunction()

IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  IF(LIBFUZZER_COMPILE_FLAGS)
    add_subdirectory(uri_parser)
  ENDIF()
ENDIF()
