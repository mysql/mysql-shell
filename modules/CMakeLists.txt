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

include_directories("${CMAKE_SOURCE_DIR}/include" 
                    "${CMAKE_SOURCE_DIR}/utils" 
                    "${CMAKE_SOURCE_DIR}/mysqlxtest" 
                    "${CMAKE_SOURCE_DIR}/mysqlxtest/common"
                    "${MYSQL_INCLUDE_DIR}"
                    "${Boost_INCLUDE_DIRS}")

include_directories( "${CMAKE_SOURCE_DIR}/common/uuid/include" )

if (HAVE_V8)
  include_directories("${V8_INCLUDE_DIR}")
endif()

file(GLOB libmysqlshmods_SRC
      "*.h"
      "*.cc"
      "*.h"
      "${CMAKE_SOURCE_DIR}/src/boost_code.cc"
      "${CMAKE_SOURCE_DIR}/common/uuid/src/uuid_gen.cc"
)

if (NOT HAVE_PROTOBUF)
  # Removes from the project the X protocol files
  file(GLOB x_SRC "*mysqlx*")
  
  foreach(xfile ${x_SRC})
    list(REMOVE_ITEM libmysqlshmods_SRC "${xfile}")
  endforeach()  
else()
  include_directories("${CMAKE_BINARY_DIR}/mysqlxtest")
  INCLUDE(protobuf)
  SETUP_PROTOBUF()
endif()

if(WINDOWS_RUNTIME_MD)
  add_convenience_library_dll(mysqlshmods ${libmysqlshmods_SRC})
else()
  add_convenience_library(mysqlshmods ${libmysqlshmods_SRC})
endif()
target_link_libraries(mysqlshmods mysqlxtest)

if(WINDOWS_RUNTIME_MD)
  target_link_libraries(mysqlshmods mysqlshtypes)
  target_link_libraries(mysqlshmods ${MYSQL_LIBRARIES})
else()
  target_link_libraries(mysqlshmods mysqlsh mysqlxtest)
  target_link_libraries(mysqlshmods ${MYSQL_LIBRARIES})
endif()

if( WIN32 )
  include(msvc)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/" DESTINATION "include/modules" FILES_MATCHING PATTERN "*.h" )
install(DIRECTORY "${CMAKE_SOURCE_DIR}/utils/" DESTINATION "include/utils" FILES_MATCHING PATTERN "*.h" )
install(TARGETS mysqlshmods DESTINATION lib)

add_definitions(-DBOOST_ALL_NO_LIB )
