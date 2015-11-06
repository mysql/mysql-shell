/*
* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __mysh__utils_general__
#define __mysh__utils_general__

#include "shellcore/common.h"
#include <string>

namespace shcore
{
  bool SHCORE_PUBLIC is_valid_identifier(const std::string& name);
  std::string SHCORE_PUBLIC build_connection_string(const std::string &uri_user, const std::string &uri_password,
                                                    const std::string &uri_host, int &port, const std::string &uri_database, bool prompt_pwd,
                                                    const std::string &uri_ssl_ca, const std::string &uri_ssl_cert, const std::string &uri_ssl_key);

  void SHCORE_PUBLIC conn_str_cat_ssl_data(std::string& uri, const std::string& ssl_ca, const std::string& ssl_cert, const std::string& ssl_key);
  void SHCORE_PUBLIC parse_mysql_connstring(const std::string &connstring,
                                            std::string &protocol, std::string &user, std::string &password,
                                            std::string &host, int &port, std::string &sock,
                                            std::string &db, int &pwd_found, std::string& ssl_ca, std::string& ssl_cert, std::string& ssl_key);

  // Convenience function to simply validate the URI
  bool SHCORE_PUBLIC validate_uri(const std::string &uri);

  std::string SHCORE_PUBLIC strip_password(const std::string &connstring);

  std::string SHCORE_PUBLIC strip_ssl_args(const std::string &connstring);

  char SHCORE_PUBLIC *mysh_get_stdin_password(const char *prompt);

  // On the URI as specified on the parameters
  std::string SHCORE_PUBLIC configure_connection_string(const std::string &uri,
                                                        const std::string &user, const std::string &password,
                                                        const std::string &host, int &port,
                                                        const std::string &database, bool prompt_pwd, const std::string &ssl_ca,
                                                        const std::string &ssl_cert, const std::string &ssl_key);
}

#endif /* defined(__mysh__utils_general__) */
