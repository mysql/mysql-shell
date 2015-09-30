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
include_directories( "${CMAKE_SOURCE_DIR}/mysqlxtest")
include_directories("${CMAKE_SOURCE_DIR}/mysqlxtest/common")
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
    "${BOOST_SOURCE_CODE}"
)

#if(NOT WINDOWS_RUNTIME_MD)
  list(APPEND libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/types/ishell_core.cc")
  list(APPEND libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/common/logger/logger.cc")
#else()
#  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/types.cc")
#  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/types_cpp.cc")
#  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/object_factory.cc")
# list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/proxy_object.cc")
##  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/shell_core.cc")
#  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/common.cc")
#  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/shellcore/obj_date.cc")
#  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/utils/utils_json.h")
#  list(REMOVE_ITEM libmysqlsh_SRC "${CMAKE_SOURCE_DIR}/utils/utils_json.cc")
#endif()


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
else()
  # Removes from the project all the python files
  file(GLOB python_SRC "*python*.cc")

  foreach(pyfile ${python_SRC})
    list(REMOVE_ITEM libmysqlsh_SRC "${pyfile}")
  endforeach()
endif()

file(GLOB libmysqlshmods_SRC
      "../modules/base_*.cc"
      "../modules/base_*.h"
      "../modules/collection_*.cc"
      "../modules/collection_*.h"
      "../modules/table_*.cc"
      "../modules/table_*.h"
      "../modules/crud_*.cc"
      "../modules/crud_*.h"
      "../modules/dynamic_*.cc"
      "../modules/dynamic_*.h"
      "../modules/mod_mysqlx_*.cc"
      "../modules/mod_mysqlx_*.h"
      "../modules/mod_mysql_*.cc"
      "../modules/mod_mysql_*.h"
      "../modules/mysql.cc"
      "../modules/mysql.h"
      "../modules/mysqlxtest_utils.h"
      "${CMAKE_SOURCE_DIR}/common/uuid/src/uuid_gen.cc"
)

#Protobuf related includes
INCLUDE(protobuf)
SETUP_PROTOBUF()
include_directories("${CMAKE_BINARY_DIR}/mysqlxtest")

IF(WIN32)
  IF(WINDOWS_RUNTIME_MD)
    ADD_LIBRARY(mysqlsh SHARED ${libmysqlsh_SRC} ${libmysqlshmods_SRC})
  ELSE()
    ADD_LIBRARY(mysqlsh ${libmysqlsh_SRC} ${libmysqlshmods_SRC})
  ENDIF()

TARGET_LINK_LIBRARIES(mysqlsh
            mysqlxtest
            ${V8_LINK_LIST}
            ${PYTHON_LIBRARIES}
            ${SSL_LIBRARIES}
            ${MYSQL_LIBRARIES}
            ${BOOST_LIBRARIES}
            ${PROTOBUF_LIBRARY})

ELSE()
  ADD_CONVENIENCE_LIBRARY(clientlib ${libmysqlsh_SRC} ${libmysqlshmods_SRC})
  add_dependencies(clientlib mysqlxtest)
  SET(LIBS mysqlxtest clientlib ${V8_LINK_LIST} ${PYTHON_LIBRARIES} ${SSL_LIBRARIES} ${MYSQL_LIBRARIES} ${BOOST_LIBRARIES} ${PROTOBUF_LIBRARY})
  IF(NOT INSTALL_LIBDIR)
    SET(INSTALL_LIBDIR lib)
  endif()
  MERGE_LIBRARIES(mysqlsh STATIC ${LIBS} COMPONENT Development)
ENDIF()

if( WIN32 )
  include(msvc)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()

install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/shellcore/" DESTINATION "include/shellcore" FILES_MATCHING PATTERN "*.h" )
install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/" DESTINATION "include/modules" FILES_MATCHING PATTERN "*.h" )

if(WINDOWS_RUNTIME_MD)
  install(TARGETS mysqlsh DESTINATION bin)
endif()

add_definitions(-DSHCORE_EXPORT )
add_definitions(-DNGCOMMON_EXPORTS )
add_definitions(-DBOOST_ALL_NO_LIB )
add_definitions(-Dmyjson_EXPORTS )
if(WIN32)
  remove_definitions(-DUNICODE)
endif()
