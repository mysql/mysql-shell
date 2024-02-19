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

set(client_input_file "${CMAKE_SOURCE_DIR}/mysql-secret-store/cmake/client.cc.in")

function(add_helper_executable)
  # split into multiple lines to avoid git-hook false-positive
  set(copyright "Copyright (c) 2018, ${PACKAGE_YEAR}")
  set(owner "Oracle and/or its affiliates")
  add_definitions("-DMYSH_HELPER_COPYRIGHT=\"${copyright}, ${owner}. All rights reserved.\"")

  configure_file(${client_input_file} client.cc)

  set(exec_name "mysql-secret-store-${helper_name}")
  set(exec_src
    client.cc
    ${helper_src}
  )

  if(WIN32)
    generate_rc_file(NAME "${exec_name}.exe" DESCRIPTION "Secret store helper - ${helper_name}." OUT_RC_FILE RC_FILE)
    set(exec_src
      ${exec_src}
      "${RC_FILE}"
    )
  endif()

  include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/ext/rapidjson/include")

  include_directories(BEFORE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/mysql-secret-store/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${helper_includes}
  )

  add_shell_executable("${exec_name}" "${exec_src}" ${helper_skip_install})

  target_link_libraries("${exec_name}"
    mysql-secret-store-core
    mysqlshdk-static
    api_modules
    ${helper_libs}
    ${GCOV_LDFLAGS}
  )
endfunction(add_helper_executable)
