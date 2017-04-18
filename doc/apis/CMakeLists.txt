# Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

cmake_minimum_required (VERSION 2.8)

IF(NOT MYSH_VERSION)
  INCLUDE(${CMAKE_SOURCE_DIR}/../../version.cmake)
ENDIF()


project (mysqlsh)

file(GLOB docs_SRC
    "${CMAKE_SOURCE_DIR}/../../modules/adminapi/*.cc"
    "${CMAKE_SOURCE_DIR}/../../modules/*.cc"
)

foreach(file ${docs_SRC})
  FILE(READ "${file}" contents)
  STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
  STRING(REGEX REPLACE "\n" ";" contents "${contents}")
  STRING(REGEX REPLACE "\"\\\\;[ ]*\"" "" contents "${contents}")
  foreach(line ${contents})
    if("${line}" MATCHES "^REGISTER_HELP")

      # Retrieves the variable name to be defined
      string(FIND "${line}" "(" start)
      string(FIND "${line}" "," end)
      MATH(EXPR start "${start}+1")

      IF ("${end}" STREQUAL "-1")
        MESSAGE("${file}")
        MESSAGE("${line}")
      ENDIF()

      MATH(EXPR length "${end}-${start}")

      string(SUBSTRING "${line}" ${start} ${length} variable)

      # Now retrieves the value to be assigned
      string(FIND "${line}" "\"" start)
      string(FIND "${line}" "\")" end)
      MATH(EXPR start "${start}+1")

      IF ("${end}" STREQUAL "-1")
        MESSAGE("${file}")
        MESSAGE("${line}")
      ENDIF()


      MATH(EXPR length "${end}-${start}")
      string(SUBSTRING "${line}" ${start} ${length} value)

      # Creates the variable with the assigned value
      SET(ENV{${variable}} "${value}")
    endif()
  endforeach()
endforeach()

SET(DOX_INPUT "${CMAKE_SOURCE_DIR}/../../modules/adminapi ${CMAKE_SOURCE_DIR}/../../modules/ ${CMAKE_SOURCE_DIR}")
SET(DOX_EXAMPLE_PATH "${CMAKE_SOURCE_DIR}/../../unittest/scripts/")
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
