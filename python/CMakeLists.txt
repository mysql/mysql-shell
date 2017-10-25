# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

add_custom_command(OUTPUT mysqlprovision_exe
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mysql_gadgets mysql_gadgets
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/front_end/mysqlprovision.py __main__.py
    COMMAND zip -r mysqlprovision.zip *py mysql_gadgets
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating mysqlprovision.exe executable"
)

add_custom_command(OUTPUT mysqlprovision_bin
  COMMAND ${CMAKE_COMMAND} -E make_directory mysql_gadgets
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mysql_gadgets mysql_gadgets
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/front_end/mysqlprovision.py __main__.py
  COMMAND zip -rq ../mysqlprovision.zip *py mysql_gadgets
  COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/mysqlprovision.preamble ../mysqlprovision.zip > ${CMAKE_BINARY_DIR}/mysqlprovision
  COMMAND chmod +x ${CMAKE_BINARY_DIR}/mysqlprovision
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "creating mysqlprovision executable"
)

if(WIN32)
  add_custom_target(mysqlprovision ALL DEPENDS mysqlprovision_exe)
  install(PROGRAMS ${CMAKE_BINARY_DIR}/mysqlprovision.zip COMPONENT main DESTINATION ${INSTALL_BINDIR})
  IF (WITH_DEV)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/mysqlprovision.zip COMPONENT dev DESTINATION ${INSTALL_BINDIR})
  ENDIF()
else()
  add_custom_target(mysqlprovision ALL DEPENDS mysqlprovision_bin)
  install(PROGRAMS ${CMAKE_BINARY_DIR}/mysqlprovision COMPONENT main DESTINATION ${INSTALL_BINDIR})
  IF (WITH_DEV)
    install(PROGRAMS ${CMAKE_BINARY_DIR}/mysqlprovision COMPONENT dev DESTINATION ${INSTALL_BINDIR})
  ENDIF()
endif()
