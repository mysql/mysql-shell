# Copyright (c) 2014, 2024, Oracle and/or its affiliates.
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

IF(OPENSSL_INCLUDE_DIR)
  INCLUDE_DIRECTORIES(BEFORE ${OPENSSL_INCLUDE_DIR})
ENDIF()

SET(SHELLCORE_MINIMAL_SOURCES
  base_shell.cc
  interrupt_handler.cc
  scoped_contexts.cc
  shell_console.cc
  sigint_event.cc
  )

set(SHELLCORE_SOURCES
  ${SHELLCORE_MINIMAL_SOURCES}
  base_session.cc
  completer.cc
  credential_manager.cc
  private_key_manager.cc
  provider_script.cc
  ishell_core.cc
  shell_cli_operation.cc
  shell_cli_operation_provider.cc
  shell_cli_mapper.cc
  shell_core.cc
  shell_init.cc
  shell_notifications.cc
  shell_options.cc
  shell_prompt_options.cc
  shell_resultset_dumper.cc
  shell_sql.cc
  provider_sql.cc
  utils_help.cc
  wizard.cc)

if(HAVE_POLYGLOT)
  set(SHELLCORE_SOURCES
        ${SHELLCORE_SOURCES}
        provider_polyglot.cc
        shell_polyglot.cc)
endif()

if(HAVE_PYTHON)
  include_directories("${PYTHON_INCLUDE_DIRS}")
  set(SHELLCORE_SOURCES
        ${SHELLCORE_SOURCES}
        provider_python.cc
        shell_python.cc)
endif()

include_directories(SYSTEM ${CURL_INCLUDE_DIR})

if(WIN32)
  remove_definitions(-DUNICODE)
endif()

add_convenience_library(shellcore ${SHELLCORE_SOURCES})

add_convenience_library(shellcore-minimal ${SHELLCORE_MINIMAL_SOURCES})

ADD_STAN_TARGET(shellcore ${SHELLCORE_SOURCES})
