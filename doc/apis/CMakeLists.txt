# Copyright (c) 2014, 2022, Oracle and/or its affiliates.
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

cmake_minimum_required (VERSION 3.2)

SET(ROOT_PROJECT_DIR "${CMAKE_SOURCE_DIR}/../../")

IF(NOT MYSH_VERSION)
  INCLUDE(${CMAKE_SOURCE_DIR}/../../version.cmake)
ENDIF()


project (mysqlsh)

function (to_snake_case input output)
  string(REGEX REPLACE "([a-z0-9])([A-Z])" "\\1_\\2" value "${input}")
  string(TOLOWER "${value}" value)
  set(${output} "${value}" PARENT_SCOPE)
endfunction()

function (remove_member_indicators input output)
  string(LENGTH ${input}, len)
  math(EXPR new_len "${len} - 6 - 1")
  string(SUBSTRING "${input}" 3 ${new_len} value)
  set(${output} "${value}" PARENT_SCOPE)
endfunction()

function(resolve_member_names input output mode)
  set(${output} "${input}" PARENT_SCOPE)
  string(REGEX MATCHALL "<<<[A-Z|a-z|0-9|_]+>>>" members "${input}")
  foreach(member ${members})
    remove_member_indicators("${member}" final_member)
    if ("PY" STREQUAL "${mode}")
      to_snake_case("${final_member}" final_member)
    endif()

    # STRING(REGEX REPLACE "${member}" "${final_member}" input "${input}")
    string(REPLACE "${member}" "${final_member}" input "${input}")

    set(${output} "${input}" PARENT_SCOPE)
  endforeach()
endfunction()

function (generate_docs type)

  file(GLOB docs_SRC
      "${CMAKE_SOURCE_DIR}/../../modules/devapi/*"
      "${CMAKE_SOURCE_DIR}/../../modules/adminapi/*"
      "${CMAKE_SOURCE_DIR}/../../modules/util/*"
      "${CMAKE_SOURCE_DIR}/../../modules/*"
  )

  foreach(file ${docs_SRC})
    FILE(READ "${file}" contents)
    # To iterate the lines of the file, we must turn it into a ; separated list
    # - escape pre-existing ;
    STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
    # - turn empty lines into something non-empty, otherwise cmake skips them
    STRING(REGEX REPLACE "\n\n" "\n \n" contents "${contents}")
    STRING(REGEX REPLACE "\n\n" "\n \n" contents "${contents}")
    # - turn newlines into ;
    STRING(REGEX REPLACE "\n" ";" contents "${contents}")
    # - dunno...
    STRING(REGEX REPLACE "\"\\\\;[ ]*\"" "" contents "${contents}")
    #MESSAGE("----> ${file}")

    SET(number "0")
    SET(continued "0")
    SET(process "0")
    SET(ready_line "")

    set(ifdef 0)

    foreach(line ${contents})
      IF("${line}" STREQUAL "#ifdef DOXYGEN")
        SET(ifdef 1)
      ELSEIF("${line}" STREQUAL "#ifndef DOXYGEN")
        SET(ifdef -1)
      ELSEIF("${line}" STREQUAL "#else")
        MATH(EXPR ifdef "${ifdef} * -1")
      ELSEIF("${line}" STREQUAL "#endif")
        SET(ifdef 0)
      ELSEIF(${ifdef} EQUAL -1)
        CONTINUE()
      ENDIF()

      IF ("${continued}" STREQUAL "0")
        if("${line}" MATCHES "^REGISTER_HELP\\(")
          string(FIND "${line}" "\");" register_end REVERSE)
          IF ("${register_end}" STREQUAL "-1")
            SET(ready_line "${line}")
            SET(continued "1")
          ELSE()
            SET(ready_line "${line}")
            SET(process "1")
          ENDIF()
        elseif("${line}" MATCHES "^REGISTER_HELP_[A-Z_]*TEXT\\(")
          string(FIND "${line}" ")*\");" register_end REVERSE)
          IF("${line}" MATCHES "^REGISTER_HELP_(FUNCTION|PROPERTY|CLASS)_TEXT\\(")
            SET(tmp "2")
          ELSE()
            SET(tmp "3")
          ENDIF()
          IF ("${register_end}" STREQUAL "-1")
            SET(ready_line "${line}")
            SET(continued "${tmp}")
          ELSE()
            SET(ready_line "${line}")
            SET(process "${tmp}")
          ENDIF()
        endif()
      ELSE()
        IF("${continued}" STREQUAL "1")
          string(FIND "${line}" "\");" register_end REVERSE)
          IF("${register_end}" STREQUAL "-1")
            string(STRIP "${line}" stripped_line)
            SET(ready_line "${ready_line}${stripped_line}")
          ELSE()
            string(STRIP "${line}" stripped_line)
            SET(ready_line "${ready_line}${stripped_line}")
            STRING(REGEX REPLACE "\"\"" "" ready_line "${ready_line}")
            SET(continued "0")
            SET(process "1")
          ENDIF()
        ELSE()
          string(FIND "${line}" ")*\");" register_end REVERSE)
          IF("${register_end}" STREQUAL "-1")
            SET(ready_line "${ready_line}\n${line}")
          ELSE()
            SET(ready_line "${ready_line}\n${line}")
            SET(process "${continued}")
            SET(continued "0")
          ENDIF()
        ENDIF()
      ENDIF()

      MATH(EXPR number "${number}+1")
      # Now process a line considered complete
      IF (NOT "${process}" STREQUAL "0")
        # Retrieves the variable name to be defined
        #MESSAGE("----> ${number} : ${ready_line}")
        string(FIND "${ready_line}" "(" start)
        string(FIND "${ready_line}" "," end)
        MATH(EXPR start "${start}+1")

        IF ("${end}" STREQUAL "-1")
          MESSAGE("${file}")
          MESSAGE("${ready_line}")
        ENDIF()

        MATH(EXPR length "${end}-${start}")

        string(SUBSTRING "${ready_line}" ${start} ${length} variable)

        # Now retrieves the value to be assigned
        IF("${process}" STREQUAL "1")
          string(FIND "${ready_line}" "\"" start)
          string(FIND "${ready_line}" "\");" end REVERSE)
          MATH(EXPR start "${start}+1")
        ELSE()
          string(FIND "${ready_line}" "R\"*(\n" start)
          string(FIND "${ready_line}" ")*\");" end REVERSE)
          MATH(EXPR start "${start}+5")
        ENDIF()

        IF ("${end}" STREQUAL "-1")
          MESSAGE("${file}")
          MESSAGE("${ready_line}")
        ENDIF()

        MATH(EXPR length "${end}-${start}")
        string(SUBSTRING "${ready_line}" ${start} ${length} value)

        string(REGEX REPLACE "\\$\\{TOPIC_CONNECTION_DATA\\}" "$(TOPIC_CONNECTION_OPTIONS_DOXYGEN)\n\n$(TOPIC_CONNECTION_MORE_INFO)" value "${value}")
        string(REGEX REPLACE "\\$\\{([^}]*)\\}" "$(\\1)" value "${value}")
        SET(value_brief "")
        IF("${process}" STREQUAL "2")
          # doxygen won't handle auto-brief unless the brief part appears separately in the comment
          string(FIND "${value}" "\n \n" brief_end)
          IF(NOT "${brief_end}" STREQUAL "-1")
            string(SUBSTRING "${value}" 0 ${brief_end} value_brief)
            string(SUBSTRING "${value}" ${brief_end} -1 value)
          ENDIF()
        ENDIF()

        # Creates the variable with the assigned value
        STRING(STRIP "${variable}" stripped_variable)
        resolve_member_names("${value}" final_value "${type}")
        #MESSAGE ("VAR: '${stripped_variable}' : '${final_value}'")
        SET(ENV{${stripped_variable}} "${final_value}")
        list(APPEND all_variables ${stripped_variable})

        IF(NOT "${value_brief}" STREQUAL "")
          SET(variable_brief "${stripped_variable}_BRIEF")
          resolve_member_names("${value_brief}" final_value_brief "${type}")
          #MESSAGE ("VAR: '${variable_brief}' : '${final_value_brief}'")
          SET(ENV{${variable_brief}} "${final_value_brief}")
          list(APPEND all_variables ${variable_brief})
        ENDIF()

        SET(process "0")
      ENDIF()
    endforeach()
  endforeach()

  SET(DOX_INPUT "${CMAKE_SOURCE_DIR}/../../modules/adminapi ${CMAKE_SOURCE_DIR}/../../modules/ ${CMAKE_SOURCE_DIR}/../../modules/devapi ${CMAKE_SOURCE_DIR}/../../modules/util ${CMAKE_SOURCE_DIR}")
  SET(DOX_EXAMPLE_PATH "${CMAKE_SOURCE_DIR}/../../unittest/scripts/py_devapi/scripts/")
  SET(DOX_EXAMPLE_PATH "${DOX_EXAMPLE_PATH} ${CMAKE_SOURCE_DIR}/../../unittest/scripts/js_devapi/scripts/")
  SET(DOX_EXAMPLE_PATH "${DOX_EXAMPLE_PATH} ${CMAKE_SOURCE_DIR}/../../unittest/scripts/js_dev_api_examples/")
  SET(DOX_EXAMPLE_PATH "${DOX_EXAMPLE_PATH} ${CMAKE_SOURCE_DIR}/../../unittest/scripts/py_dev_api_examples/")
  SET(DOX_EXAMPLE_PATH "${DOX_EXAMPLE_PATH} ${CMAKE_SOURCE_DIR}/../../unittest/scripts/auto/js_devapi/scripts/")
  SET(DOX_EXAMPLE_PATH "${DOX_EXAMPLE_PATH} ${CMAKE_SOURCE_DIR}/../../unittest/scripts/auto/py_devapi/scripts/")

  SET(DOX_EXCLUDE_PATTERNS "*my_aes* *mod_dba_replicaset* *mod_dba_instan* mod_extensible_object* interactive_object_wrapper*")

  SET(DOX_LAYOUT_FILE "${CMAKE_SOURCE_DIR}/DoxygenLayout.scripting.xml")


  # JS Documentation Generation
  SET(DOX_PREDEFINED "DOXYGEN_${type}")
  SET(DOX_ENABLED_SECTIONS "DOXYGEN_${type}")

  if(SHELL_DOCS_PATH)
    SET(DOX_OUTDIR "${SHELL_DOCS_PATH}/${type}")
  else()
    SET(DOX_OUTDIR "${type}")
  endif()

  string(TIMESTAMP TODAY "%b %d, %Y")
  string(TIMESTAMP YEAR "%Y")

  # Creates the target footer file
  configure_file("${CMAKE_SOURCE_DIR}/footer.html.in"
                 "footer.html")

  # Creates the target file containing the code ready for processing
  configure_file("${CMAKE_SOURCE_DIR}/doxygen.cfg.in"
                 "doxygen_${type}.cfg")

  execute_process(COMMAND doxygen "doxygen_${type}.cfg" RESULT_VARIABLE doxygen_result)
  if(doxygen_result)
    message(FATAL_ERROR "Error running \"doxygen doxygen_${type}.cfg\": ${doxygen_result}")
  endif()

  # search for all unparsed variables

  find_program(OS_HAS_GREP "grep")

  if(OS_HAS_GREP)
    # check just the files which contain suspicious strings
    # grep recursively the output directory, look for HTML files which contain a series of upper-case characters followed by underscore
    execute_process(COMMAND "grep" "--include=*.html" "-Rl" "-e" "[[:upper:]]\\+_" "${CMAKE_BINARY_DIR}/${DOX_OUTDIR}" OUTPUT_VARIABLE html_files)
    string(REPLACE "\n" ";" html_files ${html_files})
  else()
    # check all the files
    file(GLOB_RECURSE html_files "${CMAKE_BINARY_DIR}/${DOX_OUTDIR}/*.html")
  endif()

  # remove variables which appear in the docs and are false-positives
  list(REMOVE_ITEM all_variables "ROW")

  foreach(file ${html_files})
    file(READ "${file}" contents)

    foreach(variable ${all_variables})
      string(FIND "${contents}" "${variable}" variable_found)

      if(NOT ${variable_found} EQUAL -1)
        message(WARNING "File ${file} contains unparsed variable ${variable}.")
      endif()
    endforeach()
  endforeach()

endfunction()


generate_docs("JS")
generate_docs("PY")
