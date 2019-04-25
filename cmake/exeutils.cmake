# Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

function(add_shell_executable)
  # ARGV0 - name
  # ARGV1 - sources
  # ARGV2 - (optional) if true skips install
  message(STATUS "Adding executable ${ARGV0}")
  add_executable("${ARGV0}" ${ARGV1})
  set_target_properties("${ARGV0}" PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${INSTALL_BINDIR}")
  fix_target_output_directory("${ARGV0}" "${INSTALL_BINDIR}")

  if(OPENSSL_TO_BUNDLE_DIR)
    target_link_libraries("${ARGV0}"
      "-L${OPENSSL_TO_BUNDLE_DIR}"
    )
  endif()

  if(NOT ARGV2)
    message(STATUS "Marking executable ${ARGV0} as a run-time component")
    install(TARGETS "${ARGV0}" RUNTIME COMPONENT main DESTINATION "${INSTALL_BINDIR}")
  endif()

  if(APPLE)
    if(OPENSSL_TO_BUNDLE_DIR)
      add_custom_command(TARGET "${ARGV0}" POST_BUILD
        COMMAND install_name_tool -change
                "${CRYPTO_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${CRYPTO_VERSION}"
                $<TARGET_FILE:${ARGV0}>
        COMMAND install_name_tool -change
                "${OPENSSL_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${OPENSSL_VERSION}"
                $<TARGET_FILE:${ARGV0}>
      )
    endif()
    if(BUNDLED_SHARED_PYTHON)
      get_filename_component(PYTHON_VERSION "${PYTHON_LIBRARIES}" NAME)
      add_custom_command(TARGET "${ARGV0}" POST_BUILD
        COMMAND install_name_tool -change
                "${PYTHON_VERSION}" "@loader_path/../${INSTALL_LIBDIR}/${PYTHON_VERSION}"
                $<TARGET_FILE:${ARGV0}>
      )
    endif()
  elseif(NOT WIN32)
    if(OPENSSL_TO_BUNDLE_DIR OR BUNDLED_SHARED_PYTHON)
      set_property(TARGET "${ARGV0}" PROPERTY INSTALL_RPATH "\$ORIGIN/../${INSTALL_LIBDIR}")
      set_property(TARGET "${ARGV0}" PROPERTY PROPERTY BUILD_WITH_INSTALL_RPATH TRUE)
    endif()
  endif()
endfunction()
