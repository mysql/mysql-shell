# Copyright (c) 2016, 2024, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

IF(NOT ROOT_PROJECT_DIR)
  SET(ROOT_PROJECT_DIR ${CMAKE_SOURCE_DIR})
ENDIF()

# Generate "something" to trigger cmake rerun when VERSION changes
CONFIGURE_FILE(
  ${ROOT_PROJECT_DIR}/MYSQL_VERSION
  ${CMAKE_BINARY_DIR}/MYSQL_VERSION.dep
)

# Read value for a variable from VERSION.

MACRO(MYSQL_GET_CONFIG_VALUE config_file keyword var)
 IF(NOT ${var})
   FILE (STRINGS "${config_file}" str REGEX "^[ ]*${keyword}=")
   IF(str)
     STRING(REPLACE "${keyword}=" "" str ${str})
     STRING(REGEX REPLACE  "[ ].*" ""  str "${str}")
     SET(${var} ${str})
   ENDIF()
 ENDIF()
ENDMACRO()


# Read mysql version for configure script

MACRO(GET_MYSH_VERSION)
  MYSQL_GET_CONFIG_VALUE("${ROOT_PROJECT_DIR}/MYSQL_VERSION" "MYSQL_VERSION_MAJOR" MAJOR_VERSION)
  MYSQL_GET_CONFIG_VALUE("${ROOT_PROJECT_DIR}/MYSQL_VERSION" "MYSQL_VERSION_MINOR" MINOR_VERSION)
  MYSQL_GET_CONFIG_VALUE("${ROOT_PROJECT_DIR}/MYSQL_VERSION" "MYSQL_VERSION_PATCH" PATCH_VERSION)
  MYSQL_GET_CONFIG_VALUE("${ROOT_PROJECT_DIR}/MYSQL_VERSION" "MYSQL_VERSION_EXTRA" EXTRA_VERSION)

  IF(NOT DEFINED MAJOR_VERSION OR
     NOT DEFINED MINOR_VERSION OR
     NOT DEFINED PATCH_VERSION)
     MESSAGE(FATAL_ERROR "MYSQL_VERSION file cannot be parsed.")
  ENDIF()

  SET(MYSH_VERSION
    "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}${EXTRA_VERSION}")
  MESSAGE(STATUS "MySQL Shell ${MYSH_VERSION}")
  SET(MYSH_BASE_VERSION
    "${MAJOR_VERSION}.${MINOR_VERSION}" CACHE INTERNAL "MySQL Shell Base version")
  SET(MYSH_NO_DASH_VERSION
    "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}")
  SET(CPACK_PACKAGE_VERSION_MAJOR ${MAJOR_VERSION})
  SET(CPACK_PACKAGE_VERSION_MINOR ${MINOR_VERSION})
  SET(CPACK_PACKAGE_VERSION_PATCH ${PATCH_VERSION})
ENDMACRO()

MACRO(GET_MYSQL_VERSION)
  IF(MYSQL_SOURCE_DIR)
    FILE(TO_CMAKE_PATH "${MYSQL_SOURCE_DIR}" MYSQL_SRC_ROOT)
    MYSQL_GET_CONFIG_VALUE("${MYSQL_SRC_ROOT}/MYSQL_VERSION" "MYSQL_VERSION_MAJOR" MYSQL_MAJOR_VERSION)
    MYSQL_GET_CONFIG_VALUE("${MYSQL_SRC_ROOT}/MYSQL_VERSION" "MYSQL_VERSION_MINOR" MYSQL_MINOR_VERSION)
    MYSQL_GET_CONFIG_VALUE("${MYSQL_SRC_ROOT}/MYSQL_VERSION" "MYSQL_VERSION_PATCH" MYSQL_PATCH_VERSION)
    MYSQL_GET_CONFIG_VALUE("${MYSQL_SRC_ROOT}/MYSQL_VERSION" "MYSQL_VERSION_EXTRA" MYSQL_EXTRA_VERSION)

    IF(NOT DEFINED MYSQL_MAJOR_VERSION OR
      NOT DEFINED MYSQL_MINOR_VERSION OR
      NOT DEFINED MYSQL_PATCH_VERSION)
      MESSAGE(FATAL_ERROR "MYSQL_VERSION file for server cannot be parsed.")
    ENDIF()

    SET(MYSQL_VERSION
      "${MYSQL_MAJOR_VERSION}.${MYSQL_MINOR_VERSION}.${MYSQL_PATCH_VERSION}${MYSQL_EXTRA_VERSION}")
  ELSEIF(BUNDLED_MYSQL_CONFIG_EDITOR)
    EXECUTE_PROCESS(
      COMMAND ${BUNDLED_MYSQL_CONFIG_EDITOR} --version
      OUTPUT_VARIABLE _mysql_version
      OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Clean up so only numeric, in case of "-alpha" or similar
    STRING(REGEX MATCHALL "([0-9]+.[0-9]+.[0-9]+)" MYSQL_VERSION "${_mysql_version}")
  ENDIF()

  IF(MYSQL_VERSION)
    MESSAGE(STATUS "MySQL ${MYSQL_VERSION}")

    # To create a fully numeric version, first normalize so N.NN.NN
    STRING(REGEX REPLACE "[.]([0-9])[.]" ".0\\1." MYSQL_NUM_VERSION "${MYSQL_VERSION}")
    STRING(REGEX REPLACE "[.]([0-9])$"   ".0\\1"  MYSQL_NUM_VERSION "${MYSQL_NUM_VERSION}")

    # Finally remove the dot
    STRING(REGEX REPLACE "[.]" "" MYSQL_NUM_VERSION "${MYSQL_NUM_VERSION}")
    MESSAGE(STATUS "MySQL Num Version ${MYSQL_NUM_VERSION}")
  ENDIF()

ENDMACRO()

# Get mysql version and other interesting variables
GET_MYSH_VERSION()
GET_MYSQL_VERSION()

SET(MYSH_BUILD_ID     "$ENV{PARENT_ID}")
SET(MYSH_COMMIT_ID    "$ENV{PUSH_REVISION}")

# On Windows, AssemblyVersion does not allow slashes. Example: 1.0.5-labs
IF(WIN32)
  SET(MYSH_VERSION_WIN "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}.0")
ENDIF()
