# Copyright (c) 2017, 2021, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

INCLUDE(CMakePushCheckState)

# Only supported for Clang/llvm 6.0+, starting with this version
# -fsanitize=fuzzer will automatically enable the -fsanitize-coverage=trace-cmp
# option and link with the libFuzzer.
# Note: -fsanitize-coverage=edge is enabled by default.
# Note: -fsanitize-coverage=trace-pc-guard is not compatible with
#       -fsanitize=fuzzer starting with 7.0+, we ignore this option.
IF(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6)
  MESSAGE(STATUS "Fuzz tests require clang 6.0+")
  RETURN()
ENDIF()

SET(SAVE_CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS})
SET(SAVE_CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS})

# ld.lld: error:
# /usr/lib64/clang/7.0.1/lib/linux/libclang_rt.fuzzer-x86_64.a
# (FuzzerLoop.cpp.o): unsupported SHT_GROUP format
IF(USE_LD_LLD AND C_LD_LLD_RESULT AND CXX_LD_LLD_RESULT)
  STRING(REPLACE "-fuse-ld=lld" ""
    CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS})
  STRING(REPLACE "-fuse-ld=lld" ""
    CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS})
  STRING(REPLACE "-Wl,--gdb-index" ""
    CMAKE_C_LINK_FLAGS ${CMAKE_C_LINK_FLAGS})
  STRING(REPLACE "-Wl,--gdb-index" ""
    CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS})
ENDIF()

CMAKE_PUSH_CHECK_STATE(RESET)
SET(CMAKE_REQUIRED_LIBRARIES "-fsanitize=address,fuzzer")
SET(CMAKE_REQUIRED_FLAGS "-fsanitize=address,fuzzer")
CHECK_CXX_SOURCE_COMPILES("
extern \"C\" int LLVMFuzzerTestOneInput (void *, int)
{ return 0; }"
COMPILER_HAS_SANITIZE_FUZZER)
CMAKE_POP_CHECK_STATE()

IF(COMPILER_HAS_SANITIZE_FUZZER)
  SET(SANITIZE_COVERAGE_FLAGS "-fsanitize=address,fuzzer")
ELSE()
  MESSAGE(WARNING "Coverage is not supported. Skipping build fuzz tests.")
  SET(CMAKE_C_LINK_FLAGS ${SAVE_CMAKE_C_LINK_FLAGS})
  SET(CMAKE_CXX_LINK_FLAGS ${SAVE_CMAKE_CXX_LINK_FLAGS})
  RETURN()
ENDIF()

CMAKE_PUSH_CHECK_STATE(RESET)
SET(CMAKE_REQUIRED_FLAGS "-fprofile-instr-generate")
CHECK_CXX_COMPILER_FLAG("-fprofile-instr-generate"
  COMPILER_HAS_PROFILE_INSTR_GENERATE)
CMAKE_POP_CHECK_STATE()

IF(COMPILER_HAS_PROFILE_INSTR_GENERATE)
  CMAKE_PUSH_CHECK_STATE(RESET)
  # invalid argument '-fcoverage-mapping' only allowed
  # with '-fprofile-instr-generate'
  SET(CMAKE_REQUIRED_FLAGS "-fprofile-instr-generate")
  CHECK_CXX_COMPILER_FLAG("-fcoverage-mapping" COMPILER_HAS_COVERAGE_MAPPING)
  CMAKE_POP_CHECK_STATE()
ENDIF()

IF(COMPILER_HAS_SANITIZE_FUZZER)
  SET(LIBFUZZER_LINK_FLAGS ${SANITIZE_COVERAGE_FLAGS})
  SET(LIBFUZZER_COMPILE_FLAGS)
  LIST(APPEND LIBFUZZER_COMPILE_FLAGS ${SANITIZE_COVERAGE_FLAGS})

  IF(COMPILER_HAS_PROFILE_INSTR_GENERATE)
    LIST(APPEND LIBFUZZER_COMPILE_FLAGS -fprofile-instr-generate)
    SET(LIBFUZZER_LINK_FLAGS
      "${LIBFUZZER_LINK_FLAGS} -fprofile-instr-generate")
    IF(COMPILER_HAS_COVERAGE_MAPPING)
      LIST(APPEND LIBFUZZER_COMPILE_FLAGS -fcoverage-mapping)
    ENDIF()
  ENDIF()
ENDIF()

FUNCTION(LIBFUZZER_ADD_TEST TARGET)
  SET(OPTS)
  SET(ONE_VAL_ARGS INITIAL_CORPUS_DIR MAX_TOTAL_TIME TIMEOUT)
  SET(MULTI_VAL_ARGS)
  CMAKE_PARSE_ARGUMENTS(ARG "${OPTS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGN})


  IF(NOT DEFINED ARG_MAX_TOTAL_TIME)
    SET(ARG_MAX_TOTAL_TIME 10)
  ENDIF()
  IF(NOT DEFINED ARG_TIMEOUT)
    SET(ARG_TIMEOUT 0.2)
  ENDIF()

  SET(BINARY_CORPUS_DIR "${CMAKE_CURRENT_BINARY_DIR}/corpus/${TARGET}")
  SET(BINARY_ARTIFACT_DIR "${CMAKE_CURRENT_BINARY_DIR}/artifacts/${TARGET}")

  SET_TARGET_PROPERTIES(
    ${TARGET}
    PROPERTIES
    COMPILE_OPTIONS "${LIBFUZZER_COMPILE_FLAGS}"
    LINK_FLAGS "${LIBFUZZER_LINK_FLAGS}"
    )

  # reset the artifact and corpus dir if the binary changed.
  ADD_CUSTOM_COMMAND(TARGET ${TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${BINARY_CORPUS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_CORPUS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${BINARY_ARTIFACT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_ARTIFACT_DIR}"
  )

  IF(ARG_INITIAL_CORPUS_DIR)
    # prepare the corpus in the build-dir based on samples from the source-dir
    ADD_CUSTOM_COMMAND(TARGET ${TARGET}
      POST_BUILD
      COMMAND $<TARGET_FILE:${TARGET}>
        -merge=1
        -verbosity=0
        -merge_control_file="${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.control"
        "${ARG_INITIAL_CORPUS_DIR}" "${BINARY_CORPUS_DIR}" 2> /dev/null
      COMMENT "Preparing corpus for ${TARGET}"
      )
  ENDIF()

  # use cmake -E env to set the LLVM_PROFILE_FILE in a portable way
  ADD_TEST(${TARGET}
            ${CMAKE_BINARY_DIR}/bin/${TARGET}
            -max_total_time=${ARG_MAX_TOTAL_TIME} -timeout=${ARG_TIMEOUT}
            -artifact_prefix=${BINARY_ARTIFACT_DIR}/
            ${BINARY_CORPUS_DIR}
  )
ENDFUNCTION()
