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

#ifndef UTILS_URI_DATA_H
#define UTILS_URI_DATA_H

#include <string>
#include <map>
#include <vector>

#include "shellcore/common.h"

namespace shcore {
namespace uri {
enum TargetType {
  Tcp,
  Socket,
  Pipe
};

class SHCORE_PUBLIC Uri_data {
public:
  Uri_data();
  std::string get_scheme() { return _scheme; }
  std::string get_scheme_ext() { return _scheme_ext; }
  std::string get_user() { return _user; }
  std::string get_password();

  TargetType get_type() { return _type; }
  std::string get_host();
  bool has_port() { return _has_port; }
  int get_port();
  bool has_ssl_mode() { return _ssl_mode != 0; }
  int get_ssl_mode();

  std::string get_pipe();
  std::string get_socket();

  std::string get_db() { return _db; }

  bool has_password() { return _has_password; }

  bool has_attribute(const std::string& name) { return _attributes.find(name) != _attributes.end(); }
  std::string get_attribute(const std::string& name, size_t index = 0) { return _attributes[name][index]; }

private:
  std::string _scheme;
  std::string _scheme_ext;
  std::string _user;
  bool _has_password;
  std::string _password;
  TargetType _type;
  std::string _host;
  std::string _socket;
  std::string _pipe;
  bool _has_port;
  int _port;
  int _ssl_mode;
  std::string _db;

  std::map<std::string, std::vector< std::string > > _attributes;

  friend class Uri_parser;
};

}
}

#endif
