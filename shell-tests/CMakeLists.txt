#  Copyright (c) 2014, 2015 Oracle and/or its affiliates. All rights reserved.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; version 2 of the
#  License.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
#  02110-1301  USA

SET(INSTALL_SHELLTESTDIR "shell-tests")

IF(NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  # Enable running mxtr from build directory
  CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/mxtr.out-of-source
    ${CMAKE_CURRENT_BINARY_DIR}/mysqlx-test-run.py
    @ONLY
  )
ENDIF()


IF(UNIX)
  EXECUTE_PROCESS(
    COMMAND chmod +x  mysqlx-test-run.py
    COMMAND ${CMAKE_COMMAND} -E create_symlink
     ./mysqlx-test-run.py mxtr
    COMMAND ${CMAKE_COMMAND} -E create_symlink
     ./mysqlx-test-run.py mysqlx-test-run
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  IF(INSTALL_SHELLTESTDIR)
    INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/mxtr
      ${CMAKE_CURRENT_BINARY_DIR}/mysqlx-test-run
      DESTINATION ${INSTALL_SHELLTESTDIR}
      COMPONENT Test)
  ENDIF()
ENDIF()

