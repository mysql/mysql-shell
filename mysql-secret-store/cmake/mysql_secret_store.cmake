# Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

set(client_input_file "${CMAKE_SOURCE_DIR}/mysql-secret-store/cmake/client.cc.in")

function(add_helper_executable)
  add_definitions("-DMYSH_HELPER_COPYRIGHT=\"Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.\"")

  configure_file(${client_input_file} client.cc)

  set(exec_name "mysql-secret-store-${helper_name}")
  set(exec_src
    client.cc
    ${helper_src}
  )

  include_directories(BEFORE
    "${CMAKE_SOURCE_DIR}"
    "${CMAKE_SOURCE_DIR}/mysql-secret-store/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    ${helper_includes}
  )

  add_executable("${exec_name}" ${exec_src})
  set_target_properties("${exec_name}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}")
  fix_target_output_directory("${exec_name}" "${INSTALL_BINDIR}")

  target_link_libraries("${exec_name}"
    mysql-secret-store-core
    ${helper_libs}
    ${GCOV_LDFLAGS}
  )

  if(NOT helper_skip_install)
    install(TARGETS "${exec_name}" RUNTIME COMPONENT main DESTINATION "${INSTALL_BINDIR}")
  endif()

  if(OPENSSL_TO_BUNDLE_DIR)
    if(APPLE)
      add_custom_command(TARGET "${exec_name}" POST_BUILD
        COMMAND install_name_tool -change
                "${CRYPTO_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${CRYPTO_VERSION}"
                $<TARGET_FILE:${exec_name}>
        COMMAND install_name_tool -change
                "${OPENSSL_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${OPENSSL_VERSION}"
                $<TARGET_FILE:${exec_name}>
      )
    endif()
    if(NOT APPLE AND NOT WIN32)
       SET_PROPERTY(TARGET "${exec_name}" PROPERTY INSTALL_RPATH "\$ORIGIN/../${INSTALL_LIBDIR}")
       SET_PROPERTY(TARGET "${exec_name}" PROPERTY PROPERTY BUILD_WITH_INSTALL_RPATH TRUE)
    endif()
  endif()
endfunction(add_helper_executable)
