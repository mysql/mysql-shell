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
remove_definitions(-Dmysqlshmods_EXPORTS)

include_directories( ${MYSQL_INCLUDE_DIRS} )
include_directories( ${CMAKE_BINARY_DIR} )
include_directories( "${CMAKE_SOURCE_DIR}/mysqlxtest")
include_directories("${CMAKE_SOURCE_DIR}/mysqlxtest/common")
include_directories("${CMAKE_SOURCE_DIR}/include" "${CMAKE_SOURCE_DIR}/utils")
include_directories( "${CMAKE_SOURCE_DIR}/common/uuid/include" )
include_directories( "${CMAKE_SOURCE_DIR}/common/logger" )
include_directories( "${CMAKE_SOURCE_DIR}/common/common" )
include_directories( "${CMAKE_SOURCE_DIR}/common/process_launcher")
include_directories( "${CMAKE_SOURCE_DIR}/ext/rapidjson/include" )
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})


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

file(GLOB libmysqlshcore_SRC
    "${CMAKE_SOURCE_DIR}/include/shell/*.h"
    "${CMAKE_SOURCE_DIR}/include/shellcore/*.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_mysql_parsing.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_mysql_parsing.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_time.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_time.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_file.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_file.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_json.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_json.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_general.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_general.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_sqlstring.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_sqlstring.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_connection.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_connection.cc"
    "${CMAKE_SOURCE_DIR}/utils/base_tokenizer.h"
    "${CMAKE_SOURCE_DIR}/utils/base_tokenizer.cc"
    "${CMAKE_SOURCE_DIR}/utils/uri_parser.h"
    "${CMAKE_SOURCE_DIR}/utils/uri_parser.cc"
    "${CMAKE_SOURCE_DIR}/utils/uri_encoder.h"
    "${CMAKE_SOURCE_DIR}/utils/uri_encoder.cc"
    "${CMAKE_SOURCE_DIR}/utils/uri_data.h"
    "${CMAKE_SOURCE_DIR}/utils/uri_data.cc"
    "${CMAKE_SOURCE_DIR}/utils/utils_help.h"
    "${CMAKE_SOURCE_DIR}/utils/utils_help.cc"
    "${CMAKE_SOURCE_DIR}/interactive/interactive_global_schema.h"
    "${CMAKE_SOURCE_DIR}/interactive/interactive_global_schema.cc"
    "${CMAKE_SOURCE_DIR}/interactive/interactive_global_session.h"
    "${CMAKE_SOURCE_DIR}/interactive/interactive_global_session.cc"
    "${CMAKE_SOURCE_DIR}/shell/base_shell.cc"
    "${CMAKE_SOURCE_DIR}/shell/shell_resultset_dumper.h"
    "${CMAKE_SOURCE_DIR}/shell/shell_resultset_dumper.cc"
    "${CMAKE_SOURCE_DIR}/shell/shell_options.cc"
    "*.cc"
    "*.h"

    "${CMAKE_SOURCE_DIR}/common/uuid/src/uuid_gen.cc"
    "${CMAKE_BINARY_DIR}/shellcore/jscript_core_definitions.h"
    "${BOOST_SOURCE_CODE}"
)

#if(NOT WINDOWS_RUNTIME_MD)
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/types/ishell_core.cc")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/common/logger/logger.cc")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/common/process_launcher/process_launcher.cc")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/common/common/exception.cc")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/interactive/interactive_global_dba.h")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/interactive/interactive_global_dba.cc")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/interactive/interactive_dba_cluster.h")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/interactive/interactive_dba_cluster.cc")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/interactive/interactive_global_shell.h")
  list(APPEND libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/interactive/interactive_global_shell.cc")

#else()
#  list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/shellcore/types.cc")
#  list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/shellcore/types_cpp.cc")
#  list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/shellcore/object_factory.cc")
# list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/shellcore/proxy_object.cc")
##  list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/shellcore/shell_core.cc")
#  list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/shellcore/common.cc")
#  list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/shellcore/obj_date.cc")
#  list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/utils/utils_json.h")
#  list(REMOVE_ITEM libmysqlshcore_SRC "${CMAKE_SOURCE_DIR}/utils/utils_json.cc")
#endif()


# Configures V8 accordingly...
if ( HAVE_V8 )
  include_directories("${V8_INCLUDE_DIR}")
else()
  # Removes from the project all the jscript files
  file(GLOB js_SRC "*jscript*.cc")

  foreach(jsfile ${js_SRC})
    list(REMOVE_ITEM libmysqlshcore_SRC "${jsfile}")
  endforeach()
endif()

# Configures Python accordingly...
if ( HAVE_PYTHON )
  include_directories("${PYTHON_INCLUDE_DIR}")

  if (WITH_DEV AND WIN32)
    install(FILES ${PYTHON_LIBRARIES} DESTINATION lib COMPONENT dev)
  endif()
else()
  # Removes from the project all the python files
  file(GLOB python_SRC "*python*.cc")

  foreach(pyfile ${python_SRC})
    list(REMOVE_ITEM libmysqlshcore_SRC "${pyfile}")
  endforeach()
endif()

# Loads the metadata-model.sql file
FILE(READ "${CMAKE_SOURCE_DIR}/modules/adminapi/metadata-model.sql" MD_MODEL_SQL)

# Strips the copyright + comments from it
string(FIND "${MD_MODEL_SQL}" "CREATE DATABASE" COPYRIGHT_END)
string(SUBSTRING "${MD_MODEL_SQL}" 0 ${COPYRIGHT_END}+2 COPYRIGHT_TEXT)
string(REPLACE "${COPYRIGHT_TEXT}" "" MD_MODEL_SQL_STRIPPED "${MD_MODEL_SQL}")

# Updates format to be a C++ multiline definition
string(REPLACE "\\" "\\\\" MD_MODEL_SQL_PREPARED "${MD_MODEL_SQL_STRIPPED}")
string(REPLACE "\n" "\\n\"\n\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_PREPARED}")

# Escape the " characters in "manual"
string(REPLACE "\"manual\"" "\\\"manual\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "ssh"
string(REPLACE "\"ssh\"" "\\\"ssh\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "254616cc-fb47-11e5-aac5"
string(REPLACE "\"254616cc-fb47-11e5-aac5\"" "\\\"254616cc-fb47-11e5-aac5\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysql://host.foo.com:3306"
string(REPLACE "\"mysql://host.foo.com:3306\"" "\\\"mysql://host.foo.com:3306\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysqlx://host.foo.com:33060"
string(REPLACE "\"mysqlx://host.foo.com:33060\"" "\\\"mysqlx://host.foo.com:33060\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysql://localhost:/tmp/mysql.sock"
string(REPLACE "\"mysql://localhost:/tmp/mysql.sock\"" "\\\"mysql://localhost:/tmp/mysql.sock\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysqlx://localhost:/tmp/mysqlx.sock"
string(REPLACE "\"mysqlx://localhost:/tmp/mysqlx.sock\"" "\\\"mysqlx://localhost:/tmp/mysqlx.sock\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")

# Escape the " characters in "mysqlx://localhost:/tmp/mysqlx.sock"
string(REPLACE "\"mysqlXcom://host.foo.com:49213?channelName=<..>\"" "\\\"mysqlXcom://host.foo.com:49213?channelName=<..>\\\"" MD_MODEL_SQL_UPDATED "${MD_MODEL_SQL_UPDATED}")



# Creates the target file containing the code ready for processing
configure_file("${CMAKE_SOURCE_DIR}/include/adminapi/metadata-model_definitions.h.in"
                  "${CMAKE_BINARY_DIR}/modules/adminapi/metadata-model_definitions.h")

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
      "../modules/mod_mysqlx.cc"
      "../modules/mod_mysqlx.h"
      "../modules/mod_mysql.cc"
      "../modules/mod_mysql.h"
      "../modules/mod_mysqlx_*.cc"
      "../modules/mod_mysqlx_*.h"
      "../modules/mod_mysql_*.cc"
      "../modules/mod_mysql_*.h"
      "../modules/mod_shell.cc"
      "../modules/mod_shell.h"
      "../modules/mod_sys.cc"
      "../modules/mod_sys.h"
      "../modules/mysql_connection.cc"
      "../modules/mysql_connection.h"
      "../modules/mysqlxtest_utils.h"
      "../modules/mod_utils.h"
      "../modules/mod_utils.cc"
      "${CMAKE_SOURCE_DIR}/common/uuid/src/uuid_gen.cc"
)

include_directories(${SSL_INCLUDE_DIRS})
if(WITH_SSL STREQUAL "bundled")
  set(MY_AES_SOURCE my_aes_yassl.cc)
else()
  set(MY_AES_SOURCE my_aes_openssl.cc)
endif()

list(APPEND libmysqlshmods_SRC "${CMAKE_BINARY_DIR}/modules/adminapi/metadata-model_definitions.h"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/my_aes.cc"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/${MY_AES_SOURCE}"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba.cc"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba.h"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_cluster.cc"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_cluster.h"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_metadata_storage.cc"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_metadata_storage.h"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_provisioning_interface.cc"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_provisioning_interface.h"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_replicaset.cc"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_replicaset.h"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_common.cc"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_common.h"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_sql.cc"
            "${CMAKE_SOURCE_DIR}/modules/adminapi/mod_dba_sql.h")


#Protobuf related includes
INCLUDE(protobuf)
SETUP_PROTOBUF()
include_directories("${CMAKE_BINARY_DIR}/mysqlxtest")

IF(WIN32 AND WINDOWS_RUNTIME_MD)
  SET(MYSH_FILE_TYPE "VFT_DLL")
  SET(MYSH_PRODUCT_NAME "MySQL Shell Core Library")
  SET(MYSH_ORIGINAL_FILE_NAME "mysqlshcore.dll")
  SET(MYSH_FILE_DESCRIPTION "MySQL Shell Core Library")
  CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/res/resource.rc.in   ${CMAKE_BINARY_DIR}/mysqlshcore.rc @ONLY)

  SET(libmysqlshcore_SRC
      "${libmysqlshcore_SRC}"
      "${CMAKE_BINARY_DIR}/mysqlshcore.rc"
      )

  # Static libs don't work for C++ CLI programs somehow...
  ADD_LIBRARY(mysqlshcore SHARED ${libmysqlshcore_SRC} ${libmysqlshmods_SRC})

  TARGET_LINK_LIBRARIES(mysqlshcore
            mysqlxtest
            ${V8_LINK_LIST}
            ${PYTHON_LIBRARIES}
            ${MYSQL_LIBRARIES}
            ${BOOST_LIBRARIES}
            ${PROTOBUF_LIBRARY}
            ${SSL_LIBRARIES})
  SET(MYSQLSHCORE_LIBS mysqlshcore CACHE INTERNAL "mysqlshcore library list")
ELSE()
  ADD_LIBRARY(mysqlshcore STATIC ${libmysqlshcore_SRC} ${libmysqlshmods_SRC})

  add_dependencies(mysqlshcore mysqlxtest)
  SET(MYSQLSHCORE_LIBS mysqlshcore mysqlxtest ${V8_LINK_LIST} ${PYTHON_LIBRARIES} ${MYSQL_LIBRARIES} ${BOOST_LIBRARIES} ${PROTOBUF_LIBRARY} ${SSL_LIBRARIES} ${SSL_LIBRARIES_DL} CACHE INTERNAL "mysqlshcore library list")
ENDIF()


if( WIN32 )
  include(msvc)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()

if (WITH_DEV)
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/shellcore/" DESTINATION "${INSTALL_INCLUDEDIR}/shellcore" COMPONENT dev FILES_MATCHING PATTERN "*.h" )
    install(DIRECTORY "${CMAKE_SOURCE_DIR}/modules/" DESTINATION "${INSTALL_INCLUDEDIR}/modules" COMPONENT dev FILES_MATCHING PATTERN "*.h" )
endif()

if(WINDOWS_RUNTIME_MD)
  install(TARGETS mysqlshcore COMPONENT main DESTINATION bin)
endif()
if (WITH_DEV)
    install(TARGETS mysqlshcore COMPONENT dev DESTINATION lib)
endif()

add_definitions(-DSHCORE_EXPORT )
add_definitions(-DNGCOMMON_EXPORTS )
add_definitions(-DBOOST_ALL_NO_LIB )
add_definitions(-Dmyjson_EXPORTS )
if(WIN32)
  remove_definitions(-DUNICODE)
endif()
