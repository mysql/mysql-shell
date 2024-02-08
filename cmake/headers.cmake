# Copyright (c) 2015, 2024, Oracle and/or its affiliates.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is designed to work with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have either included with
# the program or referenced in the documentation.
#
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

#
# Infrastructure for managing public headers of the project
# =========================================================
#
# This infrastructure assumes that all public headers are located in
# a single folder of the project and its sub-folders. The top-level
# folder should contain a CMakeLists.txt with the following declarations:
#
#   SET_HEADERS(<top-level dir>)
#
#   <public header declarations>
#
#   ADD_HEADERS_TARGET()
#
# Command ADD_HEADERS_TARGET() adds to the project a target for doing
# sanity checks on headers. In GUI systems the corresponding project
# contains all declared public headers for easy access (implemented with
# SOURCE_GROUP).
#
# Public header declarations are either:
#
# ADD_HEADERS(<list of headers>) - add given headers in the current folder
# ADD_HEADERS_DIR(<dir name>)    - add all headers declared in the named sub-folder
#


#
# Determine location of files accompanying this headers.cmake module which are
# in heders/ sub-folder of the location where headers.cmake is stored
#

GET_FILENAME_COMPONENT(headers_dir ${CMAKE_CURRENT_LIST_FILE} PATH)
SET(headers_dir "${headers_dir}/headers")
#MESSAGE("headers.cmake: ${headers_dir}")


#
# Set-up header declarations with given folder as a base location for all
# public headers.
#
# A CMakeLists.txt file for a project doing sanity header checks
# is created in the corresponding build location. This file is generated
# from check.cmake.in template. When headers are declared later
# appropriate commands are written to this CMakeLists.txt file.
#
# Variable hdrs_init is used to ensure single initialization of each sub-folder
# declaring public headers (see HEADERS_DIR()).
#

MACRO(SET_HEADERS base_dir)

  GET_FILENAME_COMPONENT(hdr_base_dir ${base_dir} ABSOLUTE)
  MESSAGE(STATUS "Public headers directory: ${hdr_base_dir}")
  CONFIGURE_FILE(${headers_dir}/check.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt @ONLY)
  SET(hdrs_init 1)

ENDMACRO(SET_HEADERS)


#
# Initialize current folder for public header declarations.
#
# A sub-folder foo/bar/baz of the base headers folder adds its headers to group
# named "foo\\bar\\baz". The name of the grup and its variant of the form 
# "foo_bar_baz" are computed in hdr_group and hdr_prefix variables. If this is the
# base headers folder then hdr_group is ".". If this folder is outside of the headers
# base folder then hdr_group is "".
#
# The header group of this folder and all header sub-folders declared here are
# collected in hdr_groups variable with help of all_hdr_groups. All header files
# declare in this folder are collected in hdr_list variable.
#
# Headers declared here are added to the project doing header sanity checks. This
# is done by writing commands to CMakeLists.txt file in the corresponding build
# location. The file is initialized here.
#
# Macro HEADERS_DIR() is protected with hdrs_init variable so that it can be called
# several times but initializes given folder only once.
# 

MACRO(HEADERS_DIR)

IF(NOT hdrs_init)

  IF(NOT hdr_base_dir)
    MESSAGE(FATAL_ERROR "Header declarations without prior SET_HEADERS()")
  ENDIF()

  SET(hdrs_init 1)

  FILE(RELATIVE_PATH rel_path ${hdr_base_dir} ${CMAKE_CURRENT_SOURCE_DIR})

  IF(rel_path STREQUAL "")
    SET(rel_path ".")
  ELSEIF(rel_path MATCHES "^\\.\\.")
    #MESSAGE("outside headers dir")
    SET(rel_path "")
  ENDIF()

  STRING(REPLACE "/" "\\" hdr_group "${rel_path}")
  STRING(REPLACE "/" "_"  hdr_prefix "${rel_path}")
  SET(hdr_groups ${hdr_group})
  SET(all_hdr_groups ${hdr_group})
  SET(hdr_list "")

  FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt "# Auto generated file\n")

ENDIF()

ENDMACRO(HEADERS_DIR)


#
# Declare list of headers in the current folder as public headers of the project.
#
# Declared headers are appended to hdr_list variable and global variable headers_GGG
# is set to the current value of hdr_list where GGG is the header group of this folder.
# Also hdr_groups variable in parent scope is updated to make sure that it contains
# the current list of all header groups defined so far in this folder.
#
# Note that ADD_HEADERS() can be called several times in a given folder adding new
# headers to the list.
#

MACRO(ADD_HEADERS)

  HEADERS_DIR()

  IF(hdr_group STREQUAL "")
    MESSAGE(ERROR "Headers added from outside of header base dir"
                  " (${hdr_base_dir}) were ignored")

  ELSE()

    MESSAGE(STATUS "Adding public headers in: ${hdr_group}")

    FOREACH(hdr ${ARGV})
      GET_FILENAME_COMPONENT(hdrn ${hdr} NAME)
      MESSAGE(STATUS " - ${hdrn}")
      LIST(APPEND hdr_list ${hdr})
    ENDFOREACH(hdr)

    SET(headers_${hdr_group} ${hdr_list}
        CACHE INTERNAL "Public headers from ${hdr_group}"
        FORCE)

    SET(hdr_groups ${all_hdr_groups} PARENT_SCOPE)

  ENDIF()

ENDMACRO(ADD_HEADERS list)


#
# Add public header declarations from a named sub-folder.
#
# Variable hdr_groups in parent scope is updated to include new header
# groups introduced in the sub-folder.
#
# The sub-folder is also added to the header sanity-checks project.
#

MACRO(ADD_HEADERS_DIR dir)

  HEADERS_DIR()

  #
  # Save current value of hdr_groups in all_hdr_groups because it will be changed
  # by included sub-folder.
  #
  SET(all_hdr_groups ${hdr_groups})

  #
  # Reset hdrs_init to 0 because we want folder initialization to happen in the
  # sub-folder.
  #
  SET(hdrs_init 0)

  #
  # Headers declared in the sub-folder will be added to new header groups. Variable
  # hdr_groups will be set to hold a list of these new groups.
  #
  ADD_SUBDIRECTORY(${dir})

  SET(hdrs_init 1)

  #
  # Extend all_hdr_groups with the new groups from the sub-folder and update
  # parent's hdr_groups to hold the extended list of groups.
  #
  LIST(APPEND all_hdr_groups ${hdr_groups})
  SET(hdr_groups ${all_hdr_groups} PARENT_SCOPE)

  #
  # Add sub-folder to header sanity checks project
  #
  FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt "ADD_SUBDIRECTORY(${dir})\n")

ENDMACRO(ADD_HEADERS_DIR dir)


#
# Add all headers declared in the current folder to sanity checks project.
#
# Sanity check consists of compiling a simple file which includes given header alone.
# This file is generated from check.source.in template with @HEADER@ placeholder for
# header name (without extension).
#
# Note: this command should be executed after declaring all the headers in the folder.
#

MACRO(ADD_HEADER_CHECKS)

  #
  # For each header HHH generate test source file check_HHH.cc and add it to ceck_sources
  # list
  #

  SET(check_sources "")
  FOREACH(hdr ${hdr_list})
    GET_FILENAME_COMPONENT(hdrn ${hdr} NAME_WE)
    #MESSAGE("processing header: ${hdrn}.h")
    LIST(APPEND check_sources "check_${hdrn}.cc")
    SET(HEADER "${hdrn}")
    CONFIGURE_FILE(${headers_dir}/check.source.in "${CMAKE_CURRENT_BINARY_DIR}/check_${hdrn}.cc" @ONLY)
  ENDFOREACH(hdr)

  #
  # Add static library check_GGG (where GGG is the header groups of this folder) built from
  # test sources for all the headers. Put this folder in include path as the check project
  # can be in different location.
  #

  FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt "INCLUDE_DIRECTORIES(\"${CMAKE_CURRENT_SOURCE_DIR}\")\n")
  FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt "ADD_LIBRARY(check_${hdr_prefix} STATIC ${check_sources})\n")

ENDMACRO(ADD_HEADER_CHECKS)



#
# Add a target for public headers.
#
# Building this target will execute sanity checks for all declared public headers.
#
# Note: this macro should be called from the base headers folder and after declaring
# all public headers of the project.
#

MACRO(ADD_HEADERS_TARGET)

  #MESSAGE("groups: ${all_hdr_groups}")

  #
  # Collect all declared public headers in all_headers list. Headers are collected from
  # all header groups listed in all_hdr_groups variable. For group GGG the list of public
  # headers in that group is stored in headers_GGG variable. For each header group a
  # corresponding SOURCE_GROUP() is declared.
  #

  SET(all_headers "")
  FOREACH(group ${all_hdr_groups})
    #MESSAGE("Headers in ${group}: ${headers_${group}}")
    LIST(APPEND all_headers ${headers_${group}})
    IF(group STREQUAL ".")
      SET(group_name "Headers")
    ELSE()
      SET(group_name "Headers\\${group}")
    ENDIF()
    SOURCE_GROUP(${group_name} FILES ${headers_${group}})
  ENDFOREACH(group)

  #
  # Add the Header target which builds the sanity check project. All public headers are
  # listed as sources of this target (which gives easy access to them in GUI systems).
  # 
  ADD_CUSTOM_TARGET(Headers
    COMMAND ${CMAKE_COMMAND} --build . --clean-first
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Header checks"
    SOURCES ${all_headers}
  )

  #
  # Configure the sanity checks project. All CMakeLists.txt files defining the project
  # have been created while declaring public headers.
  #

  MESSAGE(STATUS "Configuring header checks using cmake generator: ${CMAKE_GENERATOR}")
  EXECUTE_PROCESS(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

ENDMACRO(ADD_HEADERS_TARGET)


#
# If public headers of the project use external headers and/or require some pre-processor
# definitions to work correctly then the santiy check project must define these macros
# and set required include paths. This can be done with HEADER_CHECKS_INCLUDE() and
# HEADER_CHECKS_DEFINITIONS() macros.
#


MACRO(HEADER_CHECKS_INCLUDE)

  FOREACH(dir ${ARGV})
    FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt "INCLUDE_DIRECTORIES(\"${dir}\")\n")
  ENDFOREACH(dir)

ENDMACRO(HEADER_CHECKS_INCLUDE)


MACRO(HEADER_CHECKS_DEFINITIONS)

  FOREACH(def ${ARGV})
    FILE(APPEND ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt "ADD_DEFINITIONS(${def})\n")
  ENDFOREACH(def)

ENDMACRO(HEADER_CHECKS_DEFINITIONS)
