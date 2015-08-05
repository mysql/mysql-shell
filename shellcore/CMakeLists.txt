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

remove_definitions(-Dmysqlshmods_EXPORTS)

include_directories( ${MYSQL_INCLUDE_DIRS} )
include_directories( ${CMAKE_BINARY_DIR} )
include_directories( ${CMAKE_SOURCE_DIR}/mysqlxtest)
include_directories("${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/utils" "${Boost_INCLUDE_DIRS}")
include_directories( "${CMAKE_SOURCE_DIR}/common/uuid/include" )
include_directories( "${CMAKE_SOURCE_DIR}/common/myjson/include" )
include_directories( "${CMAKE_SOURCE_DIR}/common/logger" )
include_directories( "${CMAKE_SOURCE_DIR}/ext/rapidjson/include" )


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
    "${CMAKE_SOURCE_DIR}/utils/utils_json.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_json.cc"
      "*.cc"
    "${CMAKE_SOURCE_DIR}/common/uuid/src/uuid_gen.cc"
    "${CMAKE_SOURCE_DIR}/common/myjson/src/json_parser.cc"
    "${CMAKE_SOURCE_DIR}/common/myjson/src/mutable_myjson.cc"
    "${CMAKE_SOURCE_DIR}/common/myjson/src/myjson.cc"
    "${CMAKE_BINARY_DIR}/shellcore/jscript_core_definitions.h"
    "${CMAKE_SOURCE_DIR}/src/boost_code.cc"
)

if(NOT WINDOWS_RUNTIME_MD)
  list(APPEND libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/types/ishell_core.cc")
  list(APPEND libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/common/logger/logger.cc")
else()
  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/types.cc")
  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/types_cpp.cc")
  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/object_factory.cc")
  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/proxy_object.cc")
#  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/shell_core.cc")
  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/common.cc")
  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/obj_date.cc")
  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/utils/utils_json.h")
  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/utils/utils_json.cc")
endif()


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
  
  install(FILES ${PYTHON_LIBRARIES} DESTINATION lib)
  if(WINDOWS_RUNTIME_MD)
    set(PY_DLL ${PYTHON_LIBRARIES})
    string(REPLACE ".lib" ".dll" PY_DLL ${PY_DLL})
    get_filename_component(PY_FILE ${PY_DLL} NAME)
    get_filename_component(PY_DLL ${PY_DLL} DIRECTORY)
    install(FILES "${PY_DLL}/${PY_FILE}" DESTINATION lib)
  endif()  
else()
  # Removes from the project all the python files
  file(GLOB python_SRC "*python*.cc")

  foreach(pyfile ${python_SRC})
    list(REMOVE_ITEM libmysqlsh_SRC "${pyfile}")
  endforeach()
endif()

if(WINDOWS_RUNTIME_MD)
  add_convenience_library_dll(mysqlsh ${libmysqlsh_SRC})
else()
  add_convenience_library(mysqlsh ${libmysqlsh_SRC})
endif()

if ( HAVE_PYTHON )
  if ( WINDOWS_RUNTIME_MD )
    target_link_libraries(mysqlsh ${PYTHON_LIBRARIES})
  endif()
endif()

if(WINDOWS_RUNTIME_MD)
  target_link_libraries(mysqlsh mysqlshmods)
  target_link_libraries(mysqlsh mysqlshtypes)
  target_link_libraries(mysqlsh ${V8_LINK_LIST})
  target_link_libraries(mysqlsh ${MYSQL_LIBRARIES})
endif()

if( WIN32 )
  include(msvc)
  if(NOT WINDOWS_RUNTIME_MD)  
    CHANGE_MD_2_MT()
  endif()
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/shellcore/" DESTINATION "include/shellcore" FILES_MATCHING PATTERN "*.h" )
install(TARGETS mysqlsh DESTINATION lib)

add_definitions(-DSHCORE_EXPORT )
add_definitions(-DBOOST_ALL_NO_LIB )
add_definitions(-Dmyjson_EXPORTS )
if(WIN32)
  remove_definitions(-DUNICODE)
endif()
