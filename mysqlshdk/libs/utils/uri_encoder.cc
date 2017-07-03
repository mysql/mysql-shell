/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <sstream>
#include <vector>
#include <string>
#include "utils/utils_string.h"
#include "utils/uri_encoder.h"
#include "utils/utils_general.h"

namespace shcore {
namespace uri {

std::string Uri_encoder::encode(const Uri_data& data) {
  std::string ret_val;

  // TODO(rennox): We should add the logic to encode a URI string
  // from a URI_data struct

  return ret_val;
}

std::string Uri_encoder::encode_scheme(const std::string &data) {
  std::string ret_val;

  _tokenizer.reset();
  _tokenizer.set_allow_spaces(false);
  _tokenizer.set_allow_unknown_tokens(false);

  // RFC3986 Defines the next valid chars: +.-
  // However, in the Specification we only consider alphanumerics and + for
  // extensions
  _tokenizer.set_simple_tokens("+");
  _tokenizer.set_complex_token("alphanumeric", ALPHANUMERIC);

  _tokenizer.set_input(data);
  _tokenizer.process({0, data.size()-1});

  ret_val = _tokenizer.consume_token("alphanumeric");

  if (_tokenizer.tokens_available()) {
    _tokenizer.consume_token("+");
    auto ext = _tokenizer.consume_token("alphanumeric");

    if (_tokenizer.tokens_available())
      throw Uri_error(shcore::str_format("Invalid scheme format [%s], only one "
        "extension is supported", data.c_str()));
    else
      throw Uri_error(shcore::str_format("Scheme extension [%s] is not "
        "supported", ext.c_str()));
  }

  // Validate on unique supported schema formats
  // In the future we may support additional stuff, like extensions
  if (ret_val != "mysql" && ret_val != "mysqlx")
    throw Uri_error(shcore::str_format("Invalid scheme [%s], supported schemes "
      "include: mysql, mysqlx", data.c_str()));

  return ret_val;
}

std::string Uri_encoder::encode_userinfo(const std::string& data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("sub-delims", SUBDELIMITERS);

  return process(data);
}

std::string Uri_encoder::encode_host(const std::string& data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_simple_tokens(".:");
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("digits", DIGIT);
  _tokenizer.set_complex_token("hex-digits", HEXDIG);
  _tokenizer.set_complex_token("sub-delims", SUBDELIMITERS);
  _tokenizer.set_complex_token("unreserved", UNRESERVED);

  return process(data);
}

std::string Uri_encoder::encode_port(int port) {
  if (port < 0 || port > 65535)
    throw Uri_error("Port is out of the valid range: 0 - 65535");

  return std::to_string(port);
}

std::string Uri_encoder::encode_port(const std::string& data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(false);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("digits", DIGIT);

  _tokenizer.set_input(data);
  _tokenizer.process({0, data.size()-1});

  auto port_token = _tokenizer.consume_any_token();
  if (port_token.get_type() != "digits")
    throw Uri_error(shcore::str_format("Unexpected data [%s] found in port "
      "definition", port_token.get_text().c_str()));

  std::string extra_data;
  while (_tokenizer.tokens_available())
    extra_data += _tokenizer.consume_any_token().get_text();

  if (!extra_data.empty())
    throw Uri_error(shcore::str_format("Unexpected data [%s] found in port "
      "definition", extra_data.c_str()));


  return encode_port(std::stoi(port_token.get_text()));
}

std::string Uri_encoder::encode_socket(const std::string &data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("delims", std::string("!$'()*+;="));

  std::string ret_val = process(data);

  if (ret_val.find("%2F") == 0)
    ret_val.replace(0, 3, "/");

  return ret_val;
}

std::string Uri_encoder::encode_schema(const std::string& data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_simple_tokens(":@");
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("sub-delims", SUBDELIMITERS);

  return process(data);
}

std::string Uri_encoder::encode_attribute(const std::string& data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("sub-delims", std::string("!$'()*+,;"));

  return process(data);
}

std::string Uri_encoder::encode_values(const std::vector<std::string> &values,
                                       bool force_array) {
  std::string ret_val;

  bool is_array = force_array || values.size() > 1;

  if (is_array)
    ret_val += "[";

  std::vector<std::string> encoded_values;
  for (auto value : values)
    encoded_values.push_back(encode_value(value));

  ret_val += shcore::str_join(encoded_values, ",");

  if (is_array)
    ret_val += "]";

  return ret_val;
}

std::string Uri_encoder::encode_value(const std::string& data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("delims", std::string("!$'()*+;="));

  return process(data);
}

std::string Uri_encoder::pct_encode(const std::string& data) {
  std::string ret_val;

  for (auto c : data) {
    std::stringstream buffer;
    buffer << std::hex << static_cast<int>(c);
    auto hex_data = buffer.str();

    std::string upper_hex_data;
    std::locale locale;
    for (auto hex_digit : hex_data)
      upper_hex_data += std::toupper(hex_digit, locale);

    if (hex_data.size() == 1)
      ret_val += "%0" + upper_hex_data;
    else
      ret_val += "%" + upper_hex_data;
  }

  return ret_val;
}

std::string Uri_encoder::process(const std::string& data) {
  std::string ret_val;

  _tokenizer.set_input(data);
  _tokenizer.process({0, data.size()-1});
  while (_tokenizer.tokens_available()) {
    auto token = _tokenizer.consume_any_token();

    if (token.get_type() == "unknown")
      ret_val += pct_encode(token.get_text());
    else
      ret_val += token.get_text();
  }

  return ret_val;
}

}  // namespace uri
}  // namespace shcore
