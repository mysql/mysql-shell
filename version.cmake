# Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

IF(NOT ROOT_PROJECT_DIR)
  SET(ROOT_PROJECT_DIR ${CMAKE_SOURCE_DIR})
ENDIF()

# Generate "something" to trigger cmake rerun when VERSION changes
CONFIGURE_FILE(
  ${ROOT_PROJECT_DIR}/VERSION
  ${CMAKE_BINARY_DIR}/VERSION.dep
)

# Read value for a variable from VERSION.

MACRO(MYSQL_GET_CONFIG_VALUE keyword var)
 IF(NOT ${var})
   FILE (STRINGS ${ROOT_PROJECT_DIR}/VERSION str REGEX "^[ ]*${keyword}=")
   IF(str)
     STRING(REPLACE "${keyword}=" "" str ${str})
     STRING(REGEX REPLACE  "[ ].*" ""  str "${str}")
     SET(${var} ${str})
   ENDIF()
 ENDIF()
ENDMACRO()


# Read mysql version for configure script

MACRO(GET_MYSQL_VERSION)
  MYSQL_GET_CONFIG_VALUE("MYSQL_VERSION_MAJOR" MAJOR_VERSION)
  MYSQL_GET_CONFIG_VALUE("MYSQL_VERSION_MINOR" MINOR_VERSION)
  MYSQL_GET_CONFIG_VALUE("MYSQL_VERSION_PATCH" PATCH_VERSION)
  MYSQL_GET_CONFIG_VALUE("MYSQL_VERSION_EXTRA" EXTRA_VERSION)

  IF(NOT DEFINED MAJOR_VERSION OR
     NOT DEFINED MINOR_VERSION OR
     NOT DEFINED PATCH_VERSION)
    MESSAGE(FATAL_ERROR "VERSION file cannot be parsed.")
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

# Get mysql version and other interesting variables
GET_MYSQL_VERSION()

SET(MYSH_BUILD_ID     "$ENV{PARENT_ID}")

# On Windows, AssemblyVersion does not allow slashes. Example: 1.0.5-labs
IF(WIN32)
  SET(MYSH_VERSION_WIN "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}.0")
ENDIF()
