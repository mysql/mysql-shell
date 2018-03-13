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

set(CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
  ${CMAKE_MODULE_PATH}
)

include(mysql_secret_store)

add_subdirectory(core)

# helpers
if (WIN32)
  add_subdirectory(windows-credential)
else()
  # mysql_config_editor reads password from TTY, Process_launcher cannot handle
  # that on Windows
  add_subdirectory(login-path)

  if (APPLE)
    add_subdirectory(keychain)
  else()
    # secret-service is disabled, as secret-tool is not reliable
    # add_subdirectory(secret-service)
  endif()
endif()
