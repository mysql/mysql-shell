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

include_directories("${Boost_INCLUDE_DIRS}" ${CMAKE_CURRENT_SOURCE_DIR}/common)

file(GLOB libmysqlxtest_SRC
      "common/*.cc"
      "common/*.h"
      "*.cc"
      "*.h"
)

add_library(mysqlxtest ${libmysqlxtest_SRC})

MACRO(SHELL_ADD_PROTOBUF target)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
  PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${ARGN})
  set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)
  SOURCE_GROUP(Protobuf FILES ${PROTO_SRCS} ${PROTO_HDRS})
  ADD_LIBRARY(${target}_pb STATIC ${PROTO_SRCS} ${PROTO_HDRS})
  TARGET_LINK_LIBRARIES(${target}_pb ${PROTOBUF_LIBRARY})
  TARGET_LINK_LIBRARIES(${target} ${target}_pb)
ENDMACRO()

INCLUDE(protobuf)
SETUP_PROTOBUF()

file(GLOB proto_SRC
      "${CMAKE_SOURCE_DIR}/common/protocol/*.proto"
)

SHELL_ADD_PROTOBUF(mysqlxtest ${proto_SRC})

if( WIN32 )
  include(msvc)
  if(NOT WINDOWS_RUNTIME_MD)
    CHANGE_MD_2_MT()
  endif()
endif()

add_definitions(-DBOOST_ALL_NO_LIB )

install(TARGETS mysqlxtest DESTINATION lib)
install(TARGETS mysqlxtest_pb DESTINATION lib)
