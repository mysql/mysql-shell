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

include_directories(${CMAKE_SOURCE_DIR}/common/common/
                    ${CMAKE_SOURCE_DIR}/ext/rapidjson/include)
set(utils_SOURCE
    utils_mysql_parsing.h
    utils_mysql_parsing.cc
    utils_time.h
    utils_time.cc
    utils_file.h
    utils_file.cc
    utils_json.h
    utils_json.cc
    utils_general.h
    utils_general.cc
    utils_sqlstring.h
    utils_sqlstring.cc
    utils_connection.h
    utils_connection.cc
    utils_string.cc
    base_tokenizer.h
    base_tokenizer.cc
    uri_parser.h
    uri_parser.cc
    nullable.h
    ${CMAKE_SOURCE_DIR}/common/logger/logger.cc
    ${CMAKE_SOURCE_DIR}/common/process_launcher/process_launcher.cc
)

add_convenience_library(utils ${utils_SOURCE})
