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
file(GLOB upgrade_files "${CMAKE_CURRENT_SOURCE_DIR}/adminapi/common/metadata/metadata-upgrade*.sql")

# This will sort alphabetically, so it will stop working when version numbers
# start having 2 digits
list(SORT schema_files)
list(GET schema_files -1 schema_file)

# Loads the latest metadata-model.sql file
file(READ ${schema_file} SQL_DATA)

# Updates format to be a C++ raw string definition
get_filename_component(varname "${schema_file}" NAME)
string(REPLACE "metadata-model-" "k_md_model_schema_" varname ${varname})
string(REPLACE ".sql" "" varname ${varname})
string(REPLACE "." "_" varname ${varname})
set(METADATA_MODEL_DATA "R\"*(${SQL_DATA})*\"")


macro(GET_MD_VERSION schema_version _data)
  string(REGEX MATCH "schema_version [^;]+ AS SELECT ([0-9]+), ([0-9]+), ([0-9]+)" _schema_version "${_data}")
  message(STATUS "Metadata Version : ${_schema_version}")
  set(_schema_version "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")

  set(${schema_version} "${_schema_version}")
endmacro()


# Processes the latest metadata file
message(STATUS "Metadata schema file: ${schema_file}")
string(REGEX MATCH " schema_version [^;]+ AS SELECT ([0-9]+), ([0-9]+), ([0-9]+)" METADATA_MODEL_VERSION "${SQL_DATA}")
set(METADATA_MODEL_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
message(STATUS "Metadata Version: ${METADATA_MODEL_VERSION}")


SET(METADATA_SCHEMA_SCRIPTS "")

MACRO(DEFINE_UPGRADE_MODEL schema_version _data)
  # Gets the version in a format to define an env var
  string(REPLACE "." "_" ENV_VAR_VERSION ${schema_version})
  message(STATUS "Model Definition For Upgrade: METADATA_MODEL_${ENV_VAR_VERSION}")

  # Strips SQL statements before schema_version definition to be used as the
  # upgrade script
  string(FIND "${_data}" "schema_version (major, minor, patch) AS SELECT" SV_END)

  IF (NOT "${SV_END}" STREQUAL "-1")
    MATH(EXPR SV_LENGTH "${SV_END} + 55")
    string(SUBSTRING "${_data}" 0 ${SV_LENGTH} SCHEMA_VERSION_RECREATE)
    string(REPLACE "${SCHEMA_VERSION_RECREATE}" "" _upgrade_data "${_data}")

    # Defines a METADATA_MODEL_<major__<minor>_<patch> variable to be used
    # to inject the schema into an upgrade script
    SET("METADATA_MODEL_${ENV_VAR_VERSION}" "${_upgrade_data}")
  ELSE()
      string(REPLACE "CREATE DATABASE mysql_innodb_cluster_metadata;" "" _1_0_0_data "${_data}")

      # Defines a METADATA_MODEL_<major__<minor>_<patch> variable to be used
      # to inject the schema into an upgrade script
      SET("METADATA_MODEL_${ENV_VAR_VERSION}" "${_1_0_0_data}")
  ENDIF()
ENDMACRO()

MACRO(ADD_MD_SCRIPT schema_version _data)
  # Updates format to be a C++ raw string definition
  set(SQL_DATA_FIXED "{\"${schema_version}\", R\"*(${_data})*\"}\n")

  IF (METADATA_SCHEMA_SCRIPTS STREQUAL "")
    SET(METADATA_SCHEMA_SCRIPTS "${SQL_DATA_FIXED}")
  ELSE()
    SET(METADATA_SCHEMA_SCRIPTS "${METADATA_SCHEMA_SCRIPTS},\n${SQL_DATA_FIXED}")
  ENDIF()
ENDMACRO()


# Process metadata schema scripts to be executable within a metadata upgrade
# script, at this point it assumes:
# - Metadata Schema Exists
# - schema_version view exists with version 0.0.0 (So we don't touch it)
FOREACH(fn ${schema_files})
  # Loads the metadata-model-#.#.#.sql file
  file(READ ${fn} SQL_DATA)

  # Strips the copyright + comments from it
  string(FIND "${SQL_DATA}" "CREATE DATABASE" COPYRIGHT_END)
  string(SUBSTRING "${SQL_DATA}" 0 ${COPYRIGHT_END}+2 COPYRIGHT_TEXT)
  string(REPLACE "${COPYRIGHT_TEXT}" "" SQL_DATA "${SQL_DATA}")

  GET_MD_VERSION(SCHEMA_VERSION "${SQL_DATA}")

  DEFINE_UPGRADE_MODEL("${SCHEMA_VERSION}" "${SQL_DATA}")
  ADD_MD_SCRIPT("${SCHEMA_VERSION}" "${SQL_DATA}")
ENDFOREACH()

# Make list of upgrade scripts
SET(METADATA_UPGRADE_SCRIPTS "")
foreach(fn ${upgrade_files})
  # Loads the upgrade.sql file
  file(READ ${fn} SQL_DATA)

  # Strips the copyright + comments from it
  string(FIND "${SQL_DATA}" "-- Source" COPYRIGHT_END REVERSE)
  string(SUBSTRING "${SQL_DATA}" 0 ${COPYRIGHT_END} COPYRIGHT_TEXT)
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

  string(REGEX MATCH "-- Deploy: (METADATA_MODEL_[0-9|_]+)" MODEL_VARIABLE "${SQL_DATA}")

  string(REPLACE "${MODEL_VARIABLE}" "${${CMAKE_MATCH_1}}" SQL_DATA "${SQL_DATA}")

  # Updates format to be a C++ raw string definition
  set(SQL_DATA_FIXED "{\"${TARGET_VERSION}\", R\"*(${SQL_DATA})*\"}\n")

  IF (METADATA_UPGRADE_SCRIPTS STREQUAL "")
    SET(METADATA_UPGRADE_SCRIPTS "${SQL_DATA_FIXED}")
  ELSE()
    SET(METADATA_UPGRADE_SCRIPTS "${METADATA_UPGRADE_SCRIPTS},\n${SQL_DATA_FIXED}")
  ENDIF()
endforeach()

# Creates the target file containing the code ready for processing
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/adminapi/common/metadata-model_definitions.h.in"
               "${CMAKE_CURRENT_BINARY_DIR}/adminapi/common/metadata-model_definitions.h")
