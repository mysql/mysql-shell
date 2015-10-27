# Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

include_directories( "${CMAKE_SOURCE_DIR}/common/logger" )

file(GLOB libmysqlshtypes_SRC
    "${CMAKE_SOURCE_DIR}/include/shellcore/*.h"
    "${CMAKE_SOURCE_DIR}/shellcore/types.cc"
    "${CMAKE_SOURCE_DIR}/shellcore/types_cpp.cc"
    "${CMAKE_SOURCE_DIR}/shellcore/object_factory.cc"
    "${CMAKE_SOURCE_DIR}/shellcore/proxy_object.cc"
    "${CMAKE_SOURCE_DIR}/shellcore/common.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_time.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_time.cc"
    "${CMAKE_SOURCE_DIR}/src/boost_code.cc"
    "${CMAKE_SOURCE_DIR}/shellcore/obj_date.cc"
    "${CMAKE_SOURCE_DIR}/types/ishell_core.cc"
    "${CMAKE_SOURCE_DIR}/common/logger/logger.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_json.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_json.cc"
)

if(WINDOWS_RUNTIME_MD)
  add_convenience_library_dll(mysqlshtypes ${libmysqlshtypes_SRC})
endif()

if( WIN32 )
  include(msvc)
  if(WINDOWS_RUNTIME_MD)
  else()
    CHANGE_MD_2_MT()
  endif()
endif()

add_definitions(-DBOOST_ALL_NO_LIB)
add_definitions(-DNGCOMMON_EXPORTS )

install(TARGETS mysqlshtypes DESTINATION bin COMPONENT main)
install(TARGETS mysqlshtypes DESTINATION lib COMPONENT dev)
