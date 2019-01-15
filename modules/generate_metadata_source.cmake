# Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

# Goes through each metadata model SQL file and turn it into a raw
# string const declaration.

file(GLOB schema_files "${CMAKE_CURRENT_SOURCE_DIR}/adminapi/common/metadata/metadata-model*.sql")
file(GLOB upgrade_files "${CMAKE_CURRENT_SOURCE_DIR}/adminapi/common/metadata/upgrade*.sql")

# This will sort alphabetically, so it will stop working when version numbers
# start having 2 digits
list(SORT schema_files)
list(GET schema_files -1 schema_file)

# Loads the latest metadata-model.sql file
file(READ ${schema_file} SQL_DATA)

# Strips the copyright + comments from it
string(FIND "${SQL_DATA}" "CREATE DATABASE" COPYRIGHT_END)
string(SUBSTRING "${SQL_DATA}" 0 ${COPYRIGHT_END}+2 COPYRIGHT_TEXT)
string(REPLACE "${COPYRIGHT_TEXT}" "" SQL_DATA "${SQL_DATA}")

# Updates format to be a C++ raw string definition
get_filename_component(varname "${schema_file}" NAME)
string(REPLACE "metadata-model-" "k_md_model_schema_" varname ${varname})
string(REPLACE ".sql" "" varname ${varname})
string(REPLACE "." "_" varname ${varname})
set(METADATA_MODEL_DATA "R\"*(${SQL_DATA})*\"")

message(STATUS "Metadata schema file: ${schema_file}")
string(REGEX MATCH " schema_version [^;]+ AS SELECT ([0-9]+), ([0-9]+), ([0-9]+)" METADATA_MODEL_VERSION "${SQL_DATA}")
set(METADATA_MODEL_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
message(STATUS "Metadata Version (interface): ${METADATA_MODEL_VERSION}")
string(REGEX MATCH " internal_version [^;]+ AS SELECT ([0-9]+), ([0-9]+), ([0-9]+)" METADATA_MODEL_INTERNAL_VERSION "${SQL_DATA}")
if(CMAKE_MATCH_1)
  set(METADATA_MODEL_INTERNAL_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
else()
  set(METADATA_MODEL_INTERNAL_VERSION ${METADATA_MODEL_VERSION})
endif()
message(STATUS "Metadata Version (internal): ${METADATA_MODEL_INTERNAL_VERSION}")

# Make list of upgrade scripts
foreach(fn ${upgrade_files})
  # Loads the upgrade.sql file
  file(READ ${fn} SQL_DATA)

  # Strips the copyright + comments from it
  string(FIND "${SQL_DATA}" "--" COPYRIGHT_END)
  string(SUBSTRING "${SQL_DATA}" 0 ${COPYRIGHT_END}+2 COPYRIGHT_TEXT)
  string(REPLACE "${COPYRIGHT_TEXT}" "" SQL_DATA "${SQL_DATA}")

  # Extract source and target versions
  string(REGEX MATCH "-- Source: ([0-9.]*)" SOURCE_VERSION "${SQL_DATA}")
  if(NOT SOURCE_VERSION)
    message(ERROR "-- Source: line missing in ${fn}")
  else()
    set(SOURCE_VERSION ${CMAKE_MATCH_1})
  endif()
  string(REGEX MATCH "-- Target: ([0-9.]*)" TARGET_VERSION "${SQL_DATA}")
  if(NOT TARGET_VERSION)
    message(ERROR "-- Target: line missing in ${fn}")
  else()
    set(TARGET_VERSION ${CMAKE_MATCH_1})
  endif()

  # Updates format to be a C++ raw string definition
  set(SQL_DATA_FIXED "{Version(\"${SOURCE_VERSION}\"), Version(\"${TARGET_VERSION}\"), R\"*(${SQL_DATA})*\"},\n")

  # Append to the list
  set(METADATA_MODEL_UPGRADE_DATA "${METADATA_MODEL_UPGRADE_DATA}\n${SQL_DATA_FIXED}")
endforeach()


# Creates the target file containing the code ready for processing
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/adminapi/common/metadata-model_definitions.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/adminapi/common/metadata-model_definitions.h")
