# Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

SET(MYSH_MAJOR 1)
SET(MYSH_MINOR 0)
SET(MYSH_PATCH 5)
SET(MYSH_SPRINT 0)# Merge/Sprint Number (QA Tracking): 0 on Release
IF("${MYSH_SPRINT}" STREQUAL "0")
  SET(MYSH_LEVEL "-labs")
ELSE()
  SET(MYSH_LEVEL ".${MYSH_SPRINT}")
ENDIF()

SET(MYSH_BASE_VERSION "${MYSH_MAJOR}.${MYSH_MINOR}")
SET(MYSH_VERSION      "${MYSH_MAJOR}.${MYSH_MINOR}.${MYSH_PATCH}${MYSH_LEVEL}")
SET(MYSH_NO_DASH_VERSION     "${MYSH_MAJOR}.${MYSH_MINOR}.${MYSH_PATCH}")

# On Windows, AssemblyVersion does not allow slashes. Example: 1.0.5-labs
IF(WIN32)
  SET(MYSH_VERSION_WIN "${MYSH_MAJOR}.${MYSH_MINOR}.${MYSH_PATCH}.${MYSH_SPRINT}")
ENDIF()

# If MYSH_LEVEL is set, create a string that is allowed in a RPM spec "release" field
SET(RPM_EXTRA_VERSION "${MYSH_LEVEL}")
IF(RPM_EXTRA_VERSION)
  STRING(REGEX REPLACE "[^A-Za-z0-9]" "" RPM_EXTRA_VERSION "${RPM_EXTRA_VERSION}")
  SET(RPM_EXTRA_VERSION ".${RPM_EXTRA_VERSION}")
ENDIF()
