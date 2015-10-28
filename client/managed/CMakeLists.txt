# Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

# ----------------------------------------------------------------------
# Sample lib source
# ----------------------------------------------------------------------

file(GLOB lib_SRC
      "managed_shell_client.h"
      "managed_shell_client.cpp"
      "${CMAKE_SOURCE_DIR}/src/boost_code.cc"
)

add_library(managed_shell_client SHARED ${lib_SRC})

#add_definitions(-DSHELL_CLIENT_NATIVE_EXPORTS)
add_definitions(-DBOOST_ALL_NO_LIB)


include_directories("${CMAKE_SOURCE_DIR}/client")

add_dependencies(managed_shell_client shell_client)

# Managed C++ set up
set_target_properties(managed_shell_client PROPERTIES COMPILE_FLAGS "/clr")
set_target_properties(managed_shell_client PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
if(CMAKE_CXX_FLAGS_DEBUG MATCHES "/RTC1")
	string(REPLACE "/RTC1" " " CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
endif()

IF(CMAKE_CXX_FLAGS MATCHES "/EHsc")
	string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
ENDIF()

# Since target_link_libraries does transitive linking by default, 
# it is needed to clean the transitive linking list on the libraries that will be linked to the managed wrapper
# to avoid creating unnecessary dependencies that would cause the wrapper to NOT work
SET_TARGET_PROPERTIES(shell_client PROPERTIES LINK_INTERFACE_LIBRARIES "") 
SET_TARGET_PROPERTIES(mysqlsh PROPERTIES LINK_INTERFACE_LIBRARIES "") 
target_link_libraries(managed_shell_client PRIVATE shell_client mysqlsh)


# Registers the Strong Name on the development machine
# this is needed for the success compilation of the managed wrapper
execute_process(COMMAND sn.exe -i "${CMAKE_SOURCE_DIR}/client/managed/mysqlx.snk" MySQLXShell)

install(TARGETS managed_shell_client COMPONENT dev DESTINATION lib)
