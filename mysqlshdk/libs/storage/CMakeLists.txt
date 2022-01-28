# Copyright (c) 2020, 2022, Oracle and/or its affiliates.
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

include(zlib)
MYSQL_CHECK_ZLIB()

include(zstd)
MYSQL_CHECK_ZSTD()


include_directories(BEFORE "${CMAKE_SOURCE_DIR}")

if(OPENSSL_INCLUDE_DIR)
  include_directories(BEFORE "${OPENSSL_INCLUDE_DIR}")
endif()

set(library_SRC
  compressed_file.cc
  config.cc
  idirectory.cc
  ifile.cc
  utils.cc
  backend/directory.cc
  backend/file.cc
  backend/http.cc
  backend/object_storage.cc
  backend/object_storage_bucket.cc
  backend/object_storage_config.cc
  backend/oci_par_directory.cc
  backend/oci_par_directory_config.cc
  backend/memory_file.cc
  compression/gz_file.cc
  compression/zstd_file.cc
)

add_convenience_library(storage ${library_SRC})

target_link_libraries(storage
  config
  rest
  shellcore
  utils
  ${ZLIB_LIBRARY}
)
