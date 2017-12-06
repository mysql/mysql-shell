# Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

cmake_minimum_required (VERSION 2.8)

SET(ROOT_PROJECT_DIR "${CMAKE_SOURCE_DIR}/../../")

IF(NOT MYSH_VERSION)
  INCLUDE(${CMAKE_SOURCE_DIR}/../../version.cmake)
ENDIF()


project (mysqlsh)

file(GLOB docs_SRC
    "${CMAKE_SOURCE_DIR}/../../modules/devapi/*"
    "${CMAKE_SOURCE_DIR}/../../modules/adminapi/*"
    "${CMAKE_SOURCE_DIR}/../../modules/*"
)

foreach(file ${docs_SRC})
  FILE(READ "${file}" contents)
  STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
  STRING(REGEX REPLACE "\n" ";" contents "${contents}")
  STRING(REGEX REPLACE "\"\\\\;[ ]*\"" "" contents "${contents}")
  #MESSAGE("----> ${file}")

  SET(number "0")
  SET(continued "0")
  SET(process "0")
  SET(ready_line "")

  foreach(line ${contents})
    IF ("${continued}" STREQUAL "0")
      if("${line}" MATCHES "^REGISTER_HELP")
        string(FIND "${line}" "\")" register_end)
        IF ("${register_end}" STREQUAL "-1")
          SET(ready_line "${line}")
          SET(continued "1")
        ELSE()
          SET(ready_line "${line}")
          SET(process "1")
        ENDIF()
      endif()
    ELSE()
      string(FIND "${line}" "\")" register_end)
      IF ("${register_end}" STREQUAL "-1")
        string(STRIP "${line}" stripped_line)
        SET(ready_line "${ready_line}${stripped_line}")
      ELSE()
        string(STRIP "${line}" stripped_line)
        SET(ready_line "${ready_line}${stripped_line}")
        STRING(REGEX REPLACE "\"\"" "" ready_line "${ready_line}")
        SET(continued "0")
        SET(process "1")
      ENDIF()
    ENDIF()

    MATH(EXPR number "${number}+1")
    # Now process a line considered complete
    IF ("${process}" STREQUAL "1")
      # Retrieves the variable name to be defined
      #MESSAGE("----> ${number} : ${ready_line}")
      string(FIND "${ready_line}" "(" start)
      string(FIND "${ready_line}" "," end)
      MATH(EXPR start "${start}+1")

      IF ("${end}" STREQUAL "-1")
        MESSAGE("${file}")
        MESSAGE("${ready_line}")
      ENDIF()

      MATH(EXPR length "${end}-${start}")

      string(SUBSTRING "${ready_line}" ${start} ${length} variable)

      # Now retrieves the value to be assigned
      string(FIND "${ready_line}" "\"" start)
      string(FIND "${ready_line}" "\")" end)
      MATH(EXPR start "${start}+1")

      IF ("${end}" STREQUAL "-1")
        MESSAGE("${file}")
        MESSAGE("${ready_line}")
      ENDIF()


      MATH(EXPR length "${end}-${start}")
      string(SUBSTRING "${ready_line}" ${start} ${length} value)

      # Creates the variable with the assigned value
      STRING(STRIP "${variable}" stripped_variable)
      #MESSAGE ("VAR: '${stripped_variable}' : '${value}'")
      SET(ENV{${stripped_variable}} "${value}")
      SET(process "0")
    ENDIF()
  endforeach()
endforeach()

SET(DOX_INPUT "${CMAKE_SOURCE_DIR}/../../modules/adminapi ${CMAKE_SOURCE_DIR}/../../modules/ ${CMAKE_SOURCE_DIR}/../../modules/devapi ${CMAKE_SOURCE_DIR}")

SET(DOX_EXAMPLE_PATH "${CMAKE_SOURCE_DIR}/../../unittest/scripts/py_devapi/scripts/")
SET(DOX_EXAMPLE_PATH "${DOX_EXAMPLE_PATH} ${CMAKE_SOURCE_DIR}/../../unittest/scripts/js_devapi/scripts/")
SET(DOX_EXAMPLE_PATH "${DOX_EXAMPLE_PATH} ${CMAKE_SOURCE_DIR}/../../unittest/scripts/js_dev_api_examples/")
SET(DOX_EXAMPLE_PATH "${DOX_EXAMPLE_PATH} ${CMAKE_SOURCE_DIR}/../../unittest/scripts/py_dev_api_examples/")

SET(DOX_EXCLUDE_PATTERNS "*my_aes* *mod_dba_replicaset* *mod_dba_instan*")

SET(DOX_LAYOUT_FILE "${CMAKE_SOURCE_DIR}/DoxygenLayout.scripting.xml")

# JS Documentation Generation
SET(DOX_PREDEFINED "DOXYGEN_JS")
SET(DOX_ENABLED_SECTIONS "DOXYGEN_JS")

if(SHELL_DOCS_PATH)
  SET(DOX_OUTDIR "${SHELL_DOCS_PATH}/JS")
else()
  SET(DOX_OUTDIR "JS")
endif()

# Creates the target file containing the code ready for processing
configure_file("${CMAKE_SOURCE_DIR}/doxygen.cfg.in"
               "doxygen_js.cfg")

execute_process(COMMAND doxygen "doxygen_js.cfg")

# PY Documentation Generation
SET(DOX_PREDEFINED "DOXYGEN_PY")
SET(DOX_ENABLED_SECTIONS "DOXYGEN_PY")

if(SHELL_DOCS_PATH)
  SET(DOX_OUTDIR "${SHELL_DOCS_PATH}/PY")
else()
  SET(DOX_OUTDIR "PY")
endif()

# Creates the target file containing the code ready for processing
configure_file("${CMAKE_SOURCE_DIR}/doxygen.cfg.in"
               "doxygen_py.cfg")

execute_process(COMMAND doxygen "doxygen_py.cfg")
