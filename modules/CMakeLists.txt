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
                    "${CMAKE_SOURCE_DIR}/ext/rapidjson/include"
                    "${Boost_INCLUDE_DIRS}")

include_directories( "${CMAKE_SOURCE_DIR}/common/uuid/include" )

if (HAVE_V8)
  include_directories("${V8_INCLUDE_DIR}")
endif()

file(GLOB libmysqlshmods_SRC
      "base_*.cc"
      "base_*.h"
      "collection_*.cc"
      "collection_*.h"
      "table_*.cc"
      "table_*.h"
      "crud_*.cc"
      "crud_*.h"
      "dynamic_*.cc"
      "dynamic_*.h"
      "mod_mysqlx_*.cc"
      "mod_mysqlx_*.h"
      ${BOOST_SOURCE_CODE}
      "${CMAKE_SOURCE_DIR}/common/uuid/src/uuid_gen.cc"
)

if (TRUE)
  file(GLOB MYSQL_CLASSIC_SRC 
          mysql.h mysql.cc
          "mod_mysql_*.cc"
          "mod_mysql_*.h")
endif()

include_directories("${CMAKE_BINARY_DIR}/mysqlxtest")
INCLUDE(protobuf)
SETUP_PROTOBUF()

add_convenience_library(mysqlshmods ${libmysqlshmods_SRC} ${MYSQL_CLASSIC_SRC})

if( WIN32 )
  include(msvc)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/" DESTINATION "include/modules" FILES_MATCHING PATTERN "*.h" )

add_definitions(-DBOOST_ALL_NO_LIB )
