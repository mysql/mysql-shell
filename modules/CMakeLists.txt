# Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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
                    "${MYSQL_INCLUDE_DIR}"
                    "${Boost_INCLUDE_DIRS}")

if (HAVE_V8)
  include_directories("${V8_INCLUDE_DIR}")
endif()


file(GLOB libmysqlshmods_SRC
      "*.h"
      "*.cc"
)


add_convenience_library(mysqlshmods ${libmysqlshmods_SRC})

target_link_libraries(mysqlshmods mysqlsh)

if( WIN32 )
  include(msvc)
  CHANGE_MD_2_MT()
endif()

add_definitions(-DBOOST_ALL_NO_LIB )
