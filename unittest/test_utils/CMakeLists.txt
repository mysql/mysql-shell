# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

set(test_utils_SRC
    shell_base_test.h
    shell_base_test.cc
    command_line_test.h
    command_line_test.cc
    server_mock.h
    server_mock.cc

    "${PROJECT_SOURCE_DIR}/unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.cc"
    "${PROJECT_SOURCE_DIR}/unittest/test_utils/mocks/mysqlshdk/libs/db/mock_result.cc"
    "${PROJECT_SOURCE_DIR}/unittest/test_utils/mocks/mysqlshdk/libs/db/mock_row.cc"
    "${PROJECT_SOURCE_DIR}/mysqlshdk/libs/utils/process_launcher.cc"
    "${PROJECT_SOURCE_DIR}/common/common/exception.cc"
)

add_convenience_library(test_utils ${test_utils_SRC})

if(WIN32)
  include(msvc)
  if(WITH_STATIC_MSVCRT)
    CHANGE_MD_2_MT()
  endif()
endif()
