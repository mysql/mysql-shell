# Copyright (c) 2017, 2024, Oracle and/or its affiliates.
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

# We want boost 1.56.0 in order to build our boost/geometry code.
# The boost tarball is fairly big, and takes several minutes
# to download. So we recommend downloading/unpacking it
# only once, in a place visible from any bzr sandbox.
# We use only header files, so there should be no binary dependencies.

# Downloading the tarball takes about 5 minutes here at the office.
# Here are some size/time data for unpacking the tarball on my desktop:
#  size tarball-name
#  67M boost_1_55_0.tar.gz  unzipping headers    ~2 seconds 117M
#                           unzipping everything ~3 seconds 485M
# 8,8M boost_headers.tar.gz unzipping everything <1 second

MACRO(fix_target_output_directory target path)
  FOREACH(conftype ${CMAKE_CONFIGURATION_TYPES})
    STRING(TOUPPER "${conftype}" conftype_)
    SET_TARGET_PROPERTIES(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${conftype_} "${CMAKE_BINARY_DIR}/${conftype}/${path}")
  ENDFOREACH()
ENDMACRO()
