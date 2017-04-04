# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

include_directories(BEFORE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/mysqlshdk/libs
    ${CMAKE_SOURCE_DIR}/mysqlshdk/include
    ${CMAKE_SOURCE_DIR}/ext/rapidjson/include)

add_subdirectory(libs/utils)
add_subdirectory(libs/db)
add_subdirectory(scripting)
add_subdirectory(shellcore)

#TODO: ADD_VERSION_INFO
#merge_libraries(mysqlshdk SHARED
#    utils
#    db
#    scripting
#    shellcore)

merge_libraries(mysqlshdk-static STATIC
    utils
    db
    scripting
    shellcore NOINSTALL)

  #target_link_libraries(mysqlshdk
  #          mysqlxtest
  #        ${V8_LINK_LIST}
  #        ${PYTHON_LIBRARIES}
  #        ${MYSQL_LIBRARIES}
  #        ${BOOST_LIBRARIES}
  #        ${PROTOBUF_LIBRARY}
  #        ${SSL_LIBRARIES})

target_link_libraries(mysqlshdk-static
          mysqlxtest
          ${V8_LINK_LIST}
          ${PYTHON_LIBRARIES}
          ${MYSQL_LIBRARIES}
          ${BOOST_LIBRARIES}
          ${PROTOBUF_LIBRARY}
          ${SSL_LIBRARIES})
