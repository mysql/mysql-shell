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

include_directories( ${MYSQL_INCLUDE_DIRS} )
include_directories( ${CMAKE_BINARY_DIR} )
include_directories("${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/utils" "${Boost_INCLUDE_DIRS}")
include_directories( "${CMAKE_SOURCE_DIR}/common/uuid/include" )
include_directories( "${CMAKE_SOURCE_DIR}/common/myjson/include" )


# Will generate the js_core_definitions file which will contain the 
# core.js file in a variable.
if ( HAVE_V8 )
  # Loads the core.js file
  FILE(READ "${CMAKE_SOURCE_DIR}/scripting/core.js" JS_CORE_MODULE)

  # Strips the copyright notice from it
  string(FIND "${JS_CORE_MODULE}" "function" COPYRIGHT_END)
  string(SUBSTRING "${JS_CORE_MODULE}" 0 ${COPYRIGHT_END}+2 COPYRIGHT_TEXT)
  string(REPLACE "${COPYRIGHT_TEXT}" "" JS_CORE_MODULE_STRIPPED "${JS_CORE_MODULE}")

  # Updates format to be a C++ multiline definition
  string(REPLACE "\\" "\\\\" JS_CORE_MODULE_PREPARED "${JS_CORE_MODULE_STRIPPED}")
  string(REPLACE "\n" "\\n\"\n\"" JS_CORE_MODULE_UPDATED "${JS_CORE_MODULE_PREPARED}")
  
  # Creates the target file containing the code ready for processing
  configure_file("${CMAKE_SOURCE_DIR}/include/shellcore/jscript_core_definitions.h.in" 
                 "${CMAKE_BINARY_DIR}/shellcore/jscript_core_definitions.h")
endif()

file(GLOB libmysqlsh_SRC
    "${CMAKE_SOURCE_DIR}/include/shellcore/*.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_mysql_parsing.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_mysql_parsing.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_time.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_time.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_file.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_file.cc"
      "*.cc"
    "${CMAKE_SOURCE_DIR}/common/uuid/src/uuid_gen.cc"
    "${CMAKE_SOURCE_DIR}/common/myjson/src/json_parser.cc"
    "${CMAKE_SOURCE_DIR}/common/myjson/src/mutable_myjson.cc"
    "${CMAKE_SOURCE_DIR}/common/myjson/src/myjson.cc"
    "${CMAKE_BINARY_DIR}/shellcore/jscript_core_definitions.h"
)

# Configures V8 accordingly...
if ( HAVE_V8 )
  include_directories("${V8_INCLUDE_DIR}")
else()
  # Removes from the project all the jscript files
  file(GLOB js_SRC "*jscript*.cc")

  foreach(jsfile ${js_SRC})
    list(REMOVE_ITEM libmysqlsh_SRC "${jsfile}")
  endforeach()
endif()

# Configures Python accordingly...
if ( HAVE_PYTHON )
  include_directories("${PYTHON_INCLUDE_DIR}")
else()
  # Removes from the project all the python files
  file(GLOB python_SRC "*python*.cc")

  foreach(pyfile ${python_SRC})
    list(REMOVE_ITEM libmysqlsh_SRC "${pyfile}")
  endforeach()
endif()

add_convenience_library(mysqlsh ${libmysqlsh_SRC})

if( WIN32 )
  include(msvc)
  CHANGE_MD_2_MT()
endif()

add_definitions(-DSHCORE_EXPORT )
add_definitions(-DBOOST_ALL_NO_LIB )
add_definitions(-Dmyjsonlib_EXPORTS )

