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

file(GLOB UPGRADE_CHECKER_RESOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/upgrade_checker/upgrade_checker.msg)

IF(MSVC)
add_custom_command(OUTPUT upgrade_checker_msg
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/$<CONFIG>/share/mysqlsh
    DEPENDS ${UPGRADE_CHECKER_RESOURCES}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/upgrade_checker/upgrade_checker.msg ${CMAKE_BINARY_DIR}/$<CONFIG>/share/mysqlsh
)
ELSE()
add_custom_command(OUTPUT upgrade_checker_msg
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/share/mysqlsh
    DEPENDS ${UPGRADE_CHECKER_RESOURCES}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/upgrade_checker/upgrade_checker.msg ${CMAKE_BINARY_DIR}/share/mysqlsh
)
ENDIF()

add_custom_target(uc_res ALL DEPENDS upgrade_checker_msg)

IF(MSVC)
  install(PROGRAMS ${CMAKE_BINARY_DIR}/$<CONFIG>/share/mysqlsh/upgrade_checker.msg COMPONENT main DESTINATION ${INSTALL_SHAREDIR})
ELSE()
  install(PROGRAMS ${CMAKE_BINARY_DIR}/share/mysqlsh/upgrade_checker.msg COMPONENT main DESTINATION ${INSTALL_SHAREDIR})
ENDIF()
