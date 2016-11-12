# Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

##############################################################################
#
#  Handling of -DEXTRA_INSTALL from the command line, install
#  precompiled stuff from outside this build
#
##############################################################################

option(EXTRA_INSTALL "Colon separated triplets \"<file/dir>;<instdir>;<component>;...\"")

while(EXTRA_INSTALL)

  list(GET EXTRA_INSTALL 0 _dir_or_file)
  if(NOT _dir_or_file)
    break()   # EXTRA_INSTALL might have an ending ";" causing confusion
  endif()


  list(GET EXTRA_INSTALL 1 _rel_inst_dir)
  if(NOT _rel_inst_dir)
    message(FATAL_ERROR "Install location missing for \"${_dir_or_file}\" in EXTRA_INSTALL")
  endif()

  list(GET EXTRA_INSTALL 2 _component_name)
  if(NOT _component_name)
    message(FATAL_ERROR "Install component missing for \"${_dir_or_file}\" in EXTRA_INSTALL")
  endif()

  list(REMOVE_AT EXTRA_INSTALL 0 1 2)  # Remove the elements read

  # Normalize paths to the CMake way, like using forward slashes
  file(TO_CMAKE_PATH "${_dir_or_file}" _dir_or_file)
  file(TO_CMAKE_PATH "${_rel_inst_dir}" _rel_inst_dir)

  message(STATUS "Installing \"${_dir_or_file}\" into \"${_rel_inst_dir}\" component \"${_component_name}\"")

  if(IS_DIRECTORY "${_dir_or_file}")
    install(DIRECTORY "${_dir_or_file}" DESTINATION ${_rel_inst_dir} COMPONENT ${_component_name})
  elseif(EXISTS   "${_dir_or_file}")
    install(FILES     "${_dir_or_file}" DESTINATION ${_rel_inst_dir} COMPONENT ${_component_name})
  else()
    message(FATAL_ERROR "The file or directory \"${_dir_or_file}\" doesn't exist")
  endif()

endwhile()
