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


include_directories("${Boost_INCLUDE_DIRS}" 
    ${SSL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/common/yassl/taocrypt/mySTL
    ${CMAKE_CURRENT_SOURCE_DIR}/common 
    ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(${SSL_DEFINES} -DUSE_SYS_STL)

file(GLOB libmysqlxtest_SRC
      "common/*.cc"
      "common/*.h"
      "myasio/*.cc"
      "myasio/*.h"
      "*.cc"
      "*.h"
      "${CMAKE_SOURCE_DIR}/common/mysqlx/auth_mysql41.h"
      "${CMAKE_SOURCE_DIR}/common/mysqlx/auth_mysql41.cc"
)


file(GLOB proto_SRC
      "${CMAKE_SOURCE_DIR}/common/protocol/*.proto"
)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${proto_SRC})
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)
SOURCE_GROUP(Protobuf FILES ${PROTO_SRCS} ${PROTO_HDRS})

add_library(mysqlxtest ${libmysqlxtest_SRC} ${PROTO_SRCS} ${PROTO_HDRS})

target_link_libraries(mysqlxtest ${PROTOBUF_LIBRARIES})

# For now, "samples/native/lib" compiles against this library when
# creating a shared library, so we need to make sure the code is
# position independent. CMake 2.8.10 has a property flag
# CMAKE_POSITION_INDEPENDENT_CODE.
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
  set_target_properties(mysqlxtest PROPERTIES COMPILE_FLAGS "-fPIC -Wno-error=unused-value")
endif()


if( WIN32 )
  include(msvc)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()

add_definitions(-DBOOST_ALL_NO_LIB )

install(DIRECTORY "${CMAKE_SOURCE_DIR}/mysqlxtest/" DESTINATION "include/" FILES_MATCHING PATTERN "*.h" )
install(DIRECTORY "${CMAKE_SOURCE_DIR}/mysqlxtest/common/" DESTINATION "include/common/" FILES_MATCHING PATTERN "*.h" )
install(DIRECTORY "${CMAKE_BINARY_DIR}/mysqlxtest/" DESTINATION "include/" FILES_MATCHING PATTERN "*.h" )
install(TARGETS mysqlxtest DESTINATION lib)
install(FILES ${PROTOBUF_LIBRARY} DESTINATION lib)
