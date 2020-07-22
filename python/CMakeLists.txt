# Copyright (c) 2017, 2020, Oracle and/or its affiliates.
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

IF(MSVC)
add_custom_command(OUTPUT copy_python_extensions
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/$<CONFIG>/lib/mysqlsh
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/packages ${CMAKE_BINARY_DIR}/$<CONFIG>/lib/mysqlsh/python-packages
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/plugins ${CMAKE_BINARY_DIR}/$<CONFIG>/lib/mysqlsh/plugins
    COMMENT "Copying Python extensions"
)
ELSE()
add_custom_command(OUTPUT copy_python_extensions
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/lib/mysqlsh
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/packages ${CMAKE_BINARY_DIR}/lib/mysqlsh/python-packages
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/plugins ${CMAKE_BINARY_DIR}/lib/mysqlsh/plugins
    COMMENT "Copying Python extensions"
)
ENDIF()

add_custom_target(python_extensions ALL DEPENDS copy_python_extensions)

IF(MSVC)
  install(DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib/mysqlsh/python-packages COMPONENT main DESTINATION ${INSTALL_LIBDIR})
  install(DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/lib/mysqlsh/plugins COMPONENT main DESTINATION ${INSTALL_LIBDIR})
ELSE()
  install(DIRECTORY ${CMAKE_BINARY_DIR}/lib/mysqlsh/python-packages COMPONENT main DESTINATION ${INSTALL_LIBDIR})
  install(DIRECTORY ${CMAKE_BINARY_DIR}/lib/mysqlsh/plugins COMPONENT main DESTINATION ${INSTALL_LIBDIR})
ENDIF()

