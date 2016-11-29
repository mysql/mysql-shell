/*
* Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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
#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include <string>
#include <set>
#include <vector>

namespace shcore {
bool SHCORE_PUBLIC is_valid_identifier(const std::string& name);
std::string SHCORE_PUBLIC build_connection_string(Value::Map_type_ref data, bool with_password);
void SHCORE_PUBLIC conn_str_cat_ssl_data(std::string& uri, const std::string& ssl_ca, const std::string& ssl_cert, const std::string& ssl_key);
void SHCORE_PUBLIC parse_mysql_connstring(const std::string &connstring,
                                          std::string &scheme, std::string &user, std::string &password,
                                          std::string &host, int &port, std::string &sock,
                                          std::string &db, int &pwd_found, std::string& ssl_ca, std::string& ssl_cert, std::string& ssl_key,
                                          bool set_defaults = true);

Value::Map_type_ref SHCORE_PUBLIC get_connection_data(const std::string &uri, bool set_defaults = true);
void SHCORE_PUBLIC update_connection_data(Value::Map_type_ref data,
                                          const std::string &user, const char *password,
                                          const std::string &host, int &port, const std::string& sock,
                                          const std::string &database,
                                          bool ssl, const std::string &ssl_ca,
                                          const std::string &ssl_cert, const std::string &ssl_key,
                                          const std::string &auth_method);

void SHCORE_PUBLIC set_default_connection_data(Value::Map_type_ref data);

std::string SHCORE_PUBLIC get_system_user();

// Convenience function to simply validate the URI
bool SHCORE_PUBLIC validate_uri(const std::string &uri);

std::string SHCORE_PUBLIC strip_password(const std::string &connstring);

std::string SHCORE_PUBLIC strip_ssl_args(const std::string &connstring);

char SHCORE_PUBLIC *mysh_get_stdin_password(const char *prompt);

void SHCORE_PUBLIC normalize_sslca_args(std::string &ssl_ca, std::string &ssl_ca_path);

std::string SHCORE_PUBLIC join_strings(const std::set<std::string>& strings, const std::string& separator);
std::string SHCORE_PUBLIC join_strings(const std::vector<std::string>& strings, const std::string& separator);
std::vector<std::string> SHCORE_PUBLIC split_string(const std::string& input, const std::string& separator, bool compress = false);
std::vector<std::string> SHCORE_PUBLIC split_string(const std::string& input, std::vector<size_t> max_lengths);

std::string SHCORE_PUBLIC get_member_name(const std::string& name, shcore::NamingStyle style);
std::string SHCORE_PUBLIC format_text(const std::vector<std::string>& lines, size_t width, size_t left_padding, bool paragraph_per_line);
std::string SHCORE_PUBLIC format_markup_text(const std::vector<std::string>& lines, size_t width, size_t left_padding);
std::string SHCORE_PUBLIC replace_text(const std::string& source, const std::string& from, const std::string& to);
std::string get_my_hostname();
bool is_local_host(const std::string &host, bool check_hostname);
}

#endif /* defined(__mysh__utils_general__) */
