# Copyright (c) 2021, Oracle and/or its affiliates.
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

set(ssh_SOURCE
    ssh_common.cc
    ssh_connection_options.cc
    ssh_session_options.cc
    ssh_manager.cc
    ssh_session.cc
    ssh_tunnel_handler.cc
    ssh_tunnel_manager.cc
    ssh_config_reader.cc
)

get_target_property(SSH_INCLUDE_DIR ssh INTERFACE_INCLUDE_DIRECTORIES)
add_convenience_library(shellssh ${ssh_SOURCE} LINK_LIBRARIES ssh storage)
if(APPLE)
  target_include_directories(shellssh SYSTEM BEFORE PRIVATE ${SSH_INCLUDE_DIR})
else()
  target_include_directories(shellssh_objlib SYSTEM BEFORE PRIVATE ${SSH_INCLUDE_DIR})
endif()
ADD_STAN_TARGET(shellssh ${ssh_SOURCE})

