# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/*.cc)

# Implementation files that include RapidJSON headers.
# Used to supress warnings for those.
set(json_sources json_statement_reader.cc)
check_cxx_compiler_flag("-Wshadow" HAVE_SHADOW)
if(HAVE_SHADOW)
  add_compile_flags(${json_sources} COMPILE_FLAGS "-Wno-shadow")
endif()
check_cxx_compiler_flag("-Werror" HAVE_ERROR)
IF(HAVE_ERROR)
  add_compile_flags(${json_sources} COMPILE_FLAGS "-Wno-error")
ENDIF()
check_cxx_compiler_flag("-Wconversion" HAVE_CONVERSION)
IF(HAVE_CONVERSION)
  add_compile_flags(${json_sources} COMPILE_FLAGS "-Wno-conversion")
ENDIF()
check_cxx_compiler_flag("-Wpedantic" HAVE_PEDANTIC)
IF(HAVE_PEDANTIC)
  add_compile_flags(${json_sources} COMPILE_FLAGS "-Wno-pedantic")
ENDIF()


set(common_libraries)
if(WIN32)
  list(APPEND common_libraries Ws2_32.lib)
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
  list(APPEND common_libraries -lnsl -lsocket)
endif()

add_executable(mysql_server_mock ${sources})
target_include_directories(mysql_server_mock PUBLIC ${CMAKE_SOURCE_DIR}/ext/rapidjson/include/)
target_link_libraries(mysql_server_mock ${common_libraries} ${GCOV_LDFLAGS})

#set_target_output_directory(mysql_server_mock RUNTIME_OUTPUT_DIRECTORY bin)
