# Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

include_directories(
        "${CMAKE_BINARY_DIR}"
        "${CMAKE_SOURCE_DIR}/common/uuid/include"
        "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")
include_directories(${CMAKE_SOURCE_DIR}) # <--- TODO: REMOVE (something's including modules/*)


set(SHELLCORE_SOURCES
  base_shell.cc
  base_session.cc
  completer.cc
  credential_manager.cc
  provider_script.cc
  interrupt_handler.cc
  ishell_core.cc
  scoped_contexts.cc
  shell_cli_operation.cc
  shell_core.cc
  shell_init.cc
  shell_notifications.cc
  shell_options.cc
  shell_resultset_dumper.cc
  shell_sql.cc
  provider_sql.cc
  utils_help.cc
  shell_console.cc)

if(HAVE_V8)
  include_directories("${V8_INCLUDE_DIR}")
  set(SHELLCORE_SOURCES
        ${SHELLCORE_SOURCES}
        provider_javascript.cc
        shell_jscript.cc)
endif()

if(HAVE_PYTHON)
  include_directories("${PYTHON_INCLUDE_DIR}")
  set(SHELLCORE_SOURCES
        ${SHELLCORE_SOURCES}
        provider_python.cc
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
