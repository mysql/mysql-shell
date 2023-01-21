/*
 * Copyright (c) 2016, 2023, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "mysqlshdk/libs/db/uri_encoder.h"

#include <algorithm>
#include <sstream>

#include "utils/utils_general.h"
#include "utils/utils_net.h"
#include "utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace uri {
using mysqlshdk::db::uri::Type;

std::string Uri_encoder::encode_uri(const IUri_encodable &info,
                                    Tokens_mask format) {
  auto type = info.get_type();

  std::string ret_val;
  std::string scheme;
  std::string host;
  if (info.has_value(db::kScheme)) scheme = info.get(db::kScheme);
  if (info.has_value(db::kHost)) host = info.get(db::kHost);

  if (format.is_set(Tokens::Scheme) && !scheme.empty()) {
    ret_val.append(encode_scheme(scheme));
  }

  if (type == Type::File ||
      (type == Type::Generic &&
       scheme == "file")) {  // this is the simplest use case
    if (!ret_val.empty()) ret_val.append(":");

    if (format.is_set(Tokens::Transport)) {
      if (!host.empty()) ret_val.append("//").append(encode_host(host));

      if (info.has_value(db::kPath)) {
        ret_val.append(info.get(db::kPath));
      }
    }
    return ret_val;
  }

  if (!ret_val.empty()) ret_val.append("://");

  std::string user_data;
  if (format.is_set(Tokens::User) && info.has_value(db::kUser)) {
    user_data = encode_userinfo(info.get(db::kUser));

    if (format.is_set(Tokens::Password) && info.has_value(db::kPassword))
      user_data.append(":").append(encode_userinfo(info.get(db::kPassword)));
  }

  if (!user_data.empty()) {
    ret_val.append(user_data).append("@");
  }

  if (format.is_set(Tokens::Transport)) {
    if (info.has_value(db::kPipe)) {
      ret_val.append("\\\\.\\").append(encode_value(info.get(db::kPipe)));
    } else if (info.has_value(db::kSocket)) {
      ret_val.append(encode_socket(info.get(db::kSocket)));
    } else {
      if (!host.empty()) {
        ret_val.append(encode_host(host));
      } else if (scheme != "file") {
        ret_val.append("localhost");
      }
      if (info.has_value(db::kPort))
        ret_val.append(":").append(
            encode_port(std::to_string(info.get_numeric(db::kPort))));
    }
  }

  if (format.is_set(Tokens::Path)) {
    if (type == Type::DevApi) {
      if (info.has_value(db::kSchema)) {
        ret_val.append("/").append(encode_path_segment(info.get(db::kSchema)));
      }
    } else if (info.has_value(db::kPath)) {
      ret_val.append(pctencode_path(info.get(db::kPath)));
    }
  }

  // All the SSL attributes come in the format of attribute=value
  if (format.is_set(Tokens::Query)) {
    std::vector<std::string> attributes;

    auto query_attributes = info.query_attributes();
    for (const auto &attribute : query_attributes) {
      // We must ensure only the attributes supported on the query part are
      // included (i.e. discarding internal ones)
      if (!attribute.second.has_value()) {
        attributes.push_back(encode_attribute(attribute.first));
      } else {
        attributes.push_back(encode_attribute(attribute.first) + "=" +
                             encode_value(*attribute.second));
      }
    }

    if (!attributes.empty())
      ret_val.append("?").append(shcore::str_join(attributes, "&"));
  }

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
  _tokenizer.process({0, data.size() - 1});

  ret_val = _tokenizer.consume_token("alphanumeric");

  if (_tokenizer.tokens_available()) {
    _tokenizer.consume_token("+");
    auto ext = _tokenizer.consume_token("alphanumeric");

    if (_tokenizer.tokens_available())
      throw std::invalid_argument(
          shcore::str_format("Invalid scheme format "
                             "[%s], only one extension is supported",
                             data.c_str()));
    else
      throw std::invalid_argument(
          shcore::str_format("Scheme extension [%s] is "
                             "not supported",
                             ext.c_str()));
  }

  return ret_val;
}

std::string Uri_encoder::encode_userinfo(const std::string &data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("sub-delims", SUBDELIMITERS);

  return process(data);
}

std::string Uri_encoder::encode_host(const std::string &data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_simple_tokens(".:");
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("digits", DIGIT);
  _tokenizer.set_complex_token("hex-digits", HEXDIG);
  _tokenizer.set_complex_token("sub-delims", SUBDELIMITERS);
  _tokenizer.set_complex_token("unreserved", UNRESERVED);

  std::string host = process(data);

  if (mysqlshdk::utils::Net::is_ipv6(data))
    return "[" + host + "]";
  else
    return host;
}

std::string Uri_encoder::encode_port(int port) {
  if (port < 0 || port > 65535)
    throw std::invalid_argument("Port is out of the valid range: 0 - 65535");

  return std::to_string(port);
}

std::string Uri_encoder::encode_port(const std::string &data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(false);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("digits", DIGIT);

  _tokenizer.set_input(data);
  _tokenizer.process({0, data.size() - 1});

  auto port_token = _tokenizer.consume_any_token();
  if (port_token.get_type() != "digits")
    throw std::invalid_argument(
        shcore::str_format("Unexpected data [%s] found "
                           "in port definition",
                           port_token.get_text().c_str()));

  std::string extra_data;
  while (_tokenizer.tokens_available())
    extra_data += _tokenizer.consume_any_token().get_text();

  if (!extra_data.empty())
    throw std::invalid_argument(
        shcore::str_format("Unexpected data [%s] found "
                           "in port definition",
                           extra_data.c_str()));

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

  if (shcore::str_ibeginswith(ret_val, "%2F")) ret_val.replace(0, 3, "/");

  return ret_val;
}

std::string Uri_encoder::encode_path_segment(const std::string &data,
                                             bool skip_already_encoded) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_simple_tokens(":@");
  // Skipping already encoded (default behavior) will cause to not double encode
  // already percent encoded sequences
  if (skip_already_encoded) {
    _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  }
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("sub-delims", SUBDELIMITERS);

  return process(data);
}

std::string Uri_encoder::encode_attribute(const std::string &data) {
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

  if (is_array) ret_val += "[";

  std::vector<std::string> encoded_values;
  for (auto value : values) encoded_values.push_back(encode_value(value));

  ret_val += shcore::str_join(encoded_values, ",");

  if (is_array) ret_val += "]";

  return ret_val;
}

std::string Uri_encoder::encode_value(const std::string &data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("delims", std::string("!$'()*+;="));

  return process(data);
}

std::string Uri_encoder::encode_query(const std::string &data) {
  _tokenizer.reset();
  _tokenizer.set_allow_spaces(true);
  _tokenizer.set_allow_unknown_tokens(true);
  _tokenizer.set_complex_token("pct-encoded", {"%", HEXDIG, HEXDIG});
  _tokenizer.set_complex_token("unreserved", UNRESERVED);
  _tokenizer.set_complex_token("sub-delims", SUBDELIMITERS);
  _tokenizer.set_simple_tokens(":@/?");

  return process(data);
}

std::string Uri_encoder::process(const std::string &data) {
  std::string ret_val;

  _tokenizer.set_input(data);
  _tokenizer.process({0, data.size() - 1});
  while (_tokenizer.tokens_available()) {
    auto token = _tokenizer.consume_any_token();

    if (token.get_type() == "unknown")
      ret_val += shcore::pctencode(token.get_text());
    else
      ret_val += token.get_text();
  }

  return ret_val;
}

namespace {

std::string pctencode_path_segment(const std::string &s) {
  Uri_encoder encoder;
  return encoder.encode_path_segment(s, false);
}

}  // namespace

std::string pctencode_path(const std::string &s) {
  std::size_t pos = 0;
  std::string path;

  do {
    auto next_pos = s.find('/', pos);
    path += pctencode_path_segment(s.substr(pos, next_pos - pos));
    pos = next_pos;

    if (std::string::npos != pos) {
      ++pos;
      path += '/';
    }
  } while (std::string::npos != pos);

  return path;
}

std::string pctencode_query_value(const std::string &s) {
  Uri_encoder encoder;
  return encoder.encode_value(s);
}

}  // namespace uri
}  // namespace db
}  // namespace mysqlshdk
