/*
* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <functional>
#include <string>
#include <set>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif

#include "scripting/types.h"
#include "scripting/types_cpp.h"
#include "mysqlshdk/libs/db/ssl_info.h"

namespace shcore {

bool SHCORE_PUBLIC is_valid_identifier(const std::string& name);
std::string SHCORE_PUBLIC build_connection_string(Value::Map_type_ref data, bool with_password);
void SHCORE_PUBLIC conn_str_cat_ssl_data(std::string& uri, const mysqlshdk::utils::Ssl_info &ssl_info);
void SHCORE_PUBLIC parse_mysql_connstring(const std::string &connstring,
                                          std::string &scheme, std::string &user, std::string &password,
                                          std::string &host, int &port, std::string &sock,
                                          std::string &db, int &pwd_found, struct mysqlshdk::utils::Ssl_info& ssl_info, bool set_defaults = true);

Value::Map_type_ref SHCORE_PUBLIC get_connection_data(const std::string &uri, bool set_defaults = true);
void SHCORE_PUBLIC update_connection_data(Value::Map_type_ref data,
                                          const std::string &user, const char *password,
                                          const std::string &host, int &port, const std::string& sock,
                                          const std::string &database,
                                          bool ssl, const struct mysqlshdk::utils::Ssl_info& ssl_info,
                                          const std::string &auth_method);

void SHCORE_PUBLIC set_default_connection_data(Value::Map_type_ref data);

std::string SHCORE_PUBLIC get_system_user();

// Convenience function to simply validate the URI
bool SHCORE_PUBLIC validate_uri(const std::string &uri);

std::string SHCORE_PUBLIC strip_password(const std::string &connstring);

std::string SHCORE_PUBLIC strip_ssl_args(const std::string &connstring);

char SHCORE_PUBLIC *mysh_get_stdin_password(const char *prompt);

std::vector<std::string> SHCORE_PUBLIC split_string(const std::string& input, const std::string& separator, bool compress = false);
std::vector<std::string> SHCORE_PUBLIC split_string(const std::string& input, std::vector<size_t> max_lengths);
std::vector<std::string> SHCORE_PUBLIC split_string_chars(const std::string& input, const std::string& separator_chars, bool compress = false);

void SHCORE_PUBLIC split_account(const std::string& account, std::string *out_user, std::string *out_host);
std::string SHCORE_PUBLIC make_account(const std::string& user, const std::string &host);

std::string SHCORE_PUBLIC get_member_name(const std::string& name, shcore::NamingStyle style);
std::string SHCORE_PUBLIC format_text(const std::vector<std::string>& lines, size_t width, size_t left_padding, bool paragraph_per_line);
std::string SHCORE_PUBLIC format_markup_text(const std::vector<std::string>& lines, size_t width, size_t left_padding);
std::string SHCORE_PUBLIC replace_text(const std::string& source, const std::string& from, const std::string& to);
std::string get_my_hostname();
bool is_local_host(const std::string &host, bool check_hostname);


#ifdef _WIN32
// We inline these functions to avoid trouble with memory and DLL boundaries
inline std::string win_w_to_a_string(const std::wstring &wstr, int wstrl) {
  std::string str;

  // No ceonversion needed at all
  if (!wstr.empty()) {
    // Calculates the required output buffer size
    // Since -1 is used as input length, the null termination character will be included
    // on the length calculation
    int r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wstr[0], -1, nullptr,
      0, nullptr, nullptr);

    if (r > 0) {
      str = std::string(r, 0);
      // Copies the buffer, since -1 is passed as input buffer length this
      // includes the null termination character
      r = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wstr[0], -1, &str[0], r,
        nullptr, nullptr);
    }
  }

  return str;
}

inline std::wstring win_a_to_w_string(const std::string &str) {
  std::wstring wstr;

  // No conversion needed
  if (!str.empty()) {
    // Calculates the required output buffer size
    // Since -1 is used as input length, the null termination character will be included
    // on the length calculation
    int r =
      MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, &str[0], -1, nullptr, 0);

    if (r > 0) {
      wstr = std::wstring(r, 0);
      r = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, &str[0], -1, &wstr[0], r);
    }
  }
  return wstr;
}
#endif

}  // namespace shcore

#endif /* defined(__mysh__utils_general__) */
