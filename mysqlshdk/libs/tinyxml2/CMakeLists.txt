# Copyright (c) 2022 Oracle and/or its affiliates.
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

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(TINYXML2_COMPILE_OPTIONS "-Wno-double-promotion")
endif()

set(tinyxml2_SOURCES
  ${CMAKE_SOURCE_DIR}/ext/tinyxml2-9.0.0/tinyxml2.cpp
)

set(TINYXML2_INCLUDE_DIRECTORIES ${CMAKE_SOURCE_DIR}/ext/tinyxml2-9.0.0 PARENT_SCOPE)

add_convenience_library(tinyxml2
  ${tinyxml2_SOURCES}
  COMPILE_OPTIONS ${TINYXML2_COMPILE_OPTIONS}
)
