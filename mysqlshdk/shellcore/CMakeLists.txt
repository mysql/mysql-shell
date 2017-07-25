# Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

include_directories(
        "${CMAKE_BINARY_DIR}"
        "${CMAKE_SOURCE_DIR}/mysqlxtest"
        "${CMAKE_SOURCE_DIR}/mysqlxtest/common"
        "${CMAKE_SOURCE_DIR}/common/uuid/include"
        "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")
include_directories(${CMAKE_SOURCE_DIR}) # <--- TODO: REMOVE (something's including modules/*)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})


set(SHELLCORE_SOURCES
  base_shell.cc
  base_session.cc
  interrupt_handler.cc
  ishell_core.cc
  shell_core.cc
  shell_core_options.cc
  shell_notifications.cc
  shell_options.cc
  shell_resultset_dumper.cc
  shell_sql.cc
  utils_help.cc)

if(HAVE_V8)
  include_directories("${V8_INCLUDE_DIR}")
  set(SHELLCORE_SOURCES
        ${SHELLCORE_SOURCES}
        shell_jscript.cc)
endif()

if(HAVE_PYTHON)
  include_directories("${PYTHON_INCLUDE_DIR}")
  set(SHELLCORE_SOURCES
        ${SHELLCORE_SOURCES}
        shell_python.cc)
endif()

if(WIN32)
  remove_definitions(-DUNICODE)
endif()

if(WIN32)
  set(MYSH_FILE_TYPE "VFT_DLL")
  set(MYSH_PRODUCT_NAME "MySQL Shell Core Library")
  set(MYSH_ORIGINAL_FILE_NAME "mysqlshcore.dll")
  set(MYSH_FILE_DESCRIPTION "MySQL Shell Core Library")
  configure_file(${PROJECT_SOURCE_DIR}/res/resource.rc.in   ${CMAKE_BINARY_DIR}/mysqlshcore.rc @ONLY)

  set(SHELLCORE_DLL_SOURCES
        "${CMAKE_BINARY_DIR}/mysqlshcore.rc")
endif()

add_convenience_library(shellcore ${SHELLCORE_SOURCES} ${SHELLCORE_DLL_SOURCES})

ADD_STAN_TARGET(shellcore ${SHELLCORE_SOURCES})
