# Copyright (c) 2024, 2026, Oracle and/or its affiliates.
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
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

# this is a workaround to 'cmake -E copy', which is following symlinks, we want to copy them
file(COPY "${src_file}" DESTINATION "${dst_dir}")

# COPY preserves timestamps at second precision. Without a later binary
# modification the copy can remain older than its source and rebuild forever.
# Do not touch through a symlink into a dependency outside the build directory.
if(NOT IS_SYMLINK "${src_file}")
  get_filename_component(file_name "${src_file}" NAME)
  file(TOUCH_NOCREATE "${dst_dir}/${file_name}")
endif()
