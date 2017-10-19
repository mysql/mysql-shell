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

include_directories(
  ${CMAKE_SOURCE_DIR}/ext/rapidjson/include
  ${CMAKE_SOURCE_DIR}/common/uuid/include
  )

set(utils_SOURCE
    utils_mysql_parsing.cc
    utils_time.cc
    utils_file.cc
    utils_path.cc
    utils_process.cc
    utils_json.cc
    utils_general.cc
    utils_sqlstring.cc
    utils_string.cc
    utils_uuid.cc
    nullable_options.cc
    base_tokenizer.cc
    logger.cc
    process_launcher.cc
    debug.cc
    dtoa.cc
    "${CMAKE_SOURCE_DIR}/common/uuid/src/uuid_gen.cc"
)

# platform dependent implementations
IF(UNIX)
    LIST(APPEND utils_SOURCE utils_path_unix.cc)
ELSEIF(WIN32)
    LIST(APPEND utils_SOURCE utils_path_win32.cc)
ENDIF()

add_convenience_library(utils ${utils_SOURCE})
ADD_STAN_TARGET(utils ${utils_SOURCE})
