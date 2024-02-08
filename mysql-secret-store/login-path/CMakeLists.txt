# Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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

set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/mysql-secret-store/cmake
  ${CMAKE_MODULE_PATH}
)

include(mysql_secret_store)

set(helper_name "login-path")
set(helper_file "login_path_helper.h")
set(helper_class "login_path::Login_path_helper")

set(helper_src
  config_editor_invoker.cc
  login_path_helper.cc
)
set(helper_includes
  "${CMAKE_SOURCE_DIR}/mysqlshdk/include"
)
set(helper_libs
  ${MYSQL_CLIENT_LIB}
  ${MYSQL_EXTRA_LIBRARIES}
)

add_helper_executable()

# If defined, installs the mysql_config_editor binary into the shell bundle
IF(BUNDLED_MYSQL_CONFIG_EDITOR)
  # there are some issues in case of RPM builds if an ELF is patched with patchelf < 0.16.0 and then binary is stripped;
  # we don't bundle OpenSSL there, so we make patching conditional
  IF(BUNDLED_OPENSSL)
    SET(WRITE_RPATH TRUE)
  ELSE()
    SET(WRITE_RPATH FALSE)
  ENDIF()
  install_bundled_binaries(BINARIES "${BUNDLED_MYSQL_CONFIG_EDITOR}" DESTINATION "${INSTALL_LIBEXECDIR}" TARGET mysql-secret-store-login-path WRITE_RPATH ${WRITE_RPATH})
ENDIF()
