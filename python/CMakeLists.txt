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

if(NOT WITH_CONNECTOR_PYTHON AND WIN32)
  message(FATAL_ERROR "Please define -DWITH_CONNECTOR_PYTHON=<path> when calling cmake")
endif()

add_custom_command(OUTPUT mysqlprovision_exe
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mysql_gadgets mysql_gadgets
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/front_end/mysqlprovision.py __main__.py
    COMMAND ${CMAKE_COMMAND} -E chdir ${WITH_CONNECTOR_PYTHON} python setup.py install --root=${CMAKE_CURRENT_BINARY_DIR} --install-purelib=.
    COMMAND zip -r ../mysqlprovision.zip *py mysql_gadgets mysql
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Creating mysqlprovision executable"
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

add_custom_command(OUTPUT mysqlprovision_bin_with_connector
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/mysql_gadgets mysql_gadgets
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/front_end/mysqlprovision.py __main__.py
    COMMAND ${CMAKE_COMMAND} -E chdir ${WITH_CONNECTOR_PYTHON} python setup.py install --root=${CMAKE_CURRENT_BINARY_DIR} --install-purelib=.
    COMMAND zip -rq ../mysqlprovision.zip *py mysql_gadgets mysql
    COMMAND cat ${CMAKE_CURRENT_SOURCE_DIR}/mysqlprovision.preamble ../mysqlprovision.zip > ${CMAKE_BINARY_DIR}/mysqlprovision
    COMMAND chmod +x ${CMAKE_BINARY_DIR}/mysqlprovision
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Creating mysqlprovision executable"
)

if(WIN32)
  add_custom_target(mysqlprovision ALL DEPENDS mysqlprovision_exe)
  install(PROGRAMS ${CMAKE_BINARY_DIR}/mysqlprovision.zip COMPONENT main DESTINATION ${INSTALL_BINDIR})

  install(FILES "${CMAKE_SOURCE_DIR}/python/mysqlprovision.cmd" COMPONENT main DESTINATION ${INSTALL_BINDIR})

  if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_custom_command(TARGET mysqlsh PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy
                       "${CMAKE_SOURCE_DIR}/python/mysqlprovision.cmd" $<TARGET_FILE_DIR:mysqlsh>)
  endif()
else()
  if(WITH_CONNECTOR_PYTHON)
      add_custom_target(mysqlprovision ALL DEPENDS mysqlprovision_bin_with_connector)
  else()
      add_custom_target(mysqlprovision ALL DEPENDS mysqlprovision_bin)
  endif()
  install(PROGRAMS ${CMAKE_BINARY_DIR}/mysqlprovision COMPONENT main DESTINATION ${INSTALL_BINDIR})
endif()
