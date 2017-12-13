# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

file(GLOB MYSQLPROVISION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/mysql_gadgets/*py
    ${CMAKE_CURRENT_SOURCE_DIR}/mysql_gadgets/*/*py
    ${CMAKE_CURRENT_SOURCE_DIR}/front_end/mysqlprovision.py)

add_custom_command(OUTPUT mysqlprovision_zip
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/share/mysqlsh
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mysql_gadgets mysql_gadgets
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/front_end/mysqlprovision.py __main__.py
    COMMAND zip -qr ${CMAKE_BINARY_DIR}/share/mysqlsh/mysqlprovision.zip *py mysql_gadgets
    DEPENDS ${MYSQLPROVISION_SOURCES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python
    COMMENT "Creating mysqlprovision.zip"
)

add_custom_target(mysqlprovision ALL DEPENDS mysqlprovision_zip)

install(PROGRAMS ${CMAKE_BINARY_DIR}/share/mysqlsh/mysqlprovision.zip COMPONENT main DESTINATION ${INSTALL_SHAREDIR})
IF (WITH_DEV)
  install(PROGRAMS ${CMAKE_BINARY_DIR}/share/mysqlsh/mysqlprovision.zip COMPONENT dev DESTINATION ${INSTALL_SHAREDIR})
ENDIF()
