/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */
#include <cassert>
#include "uri_data.h"

namespace shcore {
namespace uri {

Uri_data::Uri_data() :_has_password(false), _has_port(false), _port(0) {
#ifdef WIN32
  _type = TargetType::Tcp;
#else
  _type = TargetType::Pipe;
#endif
}

std::string Uri_data::get_password() {
  assert(_has_password);
  return _password;
}

std::string Uri_data::get_host() {
  return _host;
}

int Uri_data::get_port() {
  assert(_has_port);
  return _port;
}

int Uri_data::get_ssl_mode() {
  assert(_ssl_mode != 0);
  return _ssl_mode;
}

std::string Uri_data::get_pipe() {
  assert(_type == Pipe);
  return _pipe;
}

std::string Uri_data::get_socket() {
  assert(_type == Socket);
  return _socket;
}

}
}