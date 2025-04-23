/*
 * Copyright (c) 2018, 2025, Oracle and/or its affiliates.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is designed to work with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have either included with
 * the program or referenced in the documentation.
 *
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <set>
#include <utility>
#include <vector>

#include "mysql-secret-store/include/helper.h"
#include "mysql-secret-store/include/mysql-secret-store/api.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/file_uri.h"
#include "mysqlshdk/libs/db/generic_uri.h"
#include "mysqlshdk/libs/db/uri_parser.h"
#include "mysqlshdk/libs/db/utils_connection.h"
#include "mysqlshdk/libs/secret-store-api/helper_invoker.h"

using mysql::secret_store::common::k_scheme_name_file;
using mysql::secret_store::common::k_scheme_name_ssh;

namespace mysql {
namespace secret_store {
namespace api {

namespace {

constexpr auto k_secret = "Secret";
constexpr auto k_secret_type = "SecretType";
constexpr auto k_secret_id = "SecretID";
// TODO(pawel): this is deprecated, remove it at some point
constexpr auto k_server_url = "ServerURL";

constexpr auto k_secret_type_password = "password";
constexpr auto k_secret_type_generic = "generic";

[[noreturn]] void throw_json_error(const std::string &msg) {
  throw std::runtime_error{"Failed to parse JSON output: " + msg};
}

std::string validate_url(const std::string &url) {
  using mysqlshdk::db::uri::Generic_uri;
  using mysqlshdk::db::uri::Type;
  using mysqlshdk::db::uri::Uri_parser;

  auto throw_exception = [](const std::string &msg) {
    throw std::runtime_error{"Invalid URL: " + msg};
  };

  Generic_uri options;
  try {
    Uri_parser parser(Type::Generic);
    parser.parse(url, &options);
  } catch (const std::exception &e) {
    throw_exception(e.what());
  }

  std::optional<std::string> scheme;
  if (options.has_value(mysqlshdk::db::kScheme))
    scheme = options.get(mysqlshdk::db::kScheme);

  if (scheme.has_value() && scheme.value() != k_scheme_name_file &&
      scheme.value() != k_scheme_name_ssh) {
    throw_exception(shcore::str_format(
        "Only ssh and file schemes are allowed: %s", url.c_str()));
  }

  if (options.has_value(mysqlshdk::db::kPassword)) {
    throw_exception(
        shcore::str_format("URL should not have a password: %s", url.c_str()));
  }

  if (std::string::npos != url.find("?")) {
    throw_exception(
        shcore::str_format("URL should not have options: %s", url.c_str()));
  }

  std::optional<std::string> user;
  if (options.has_value(mysqlshdk::db::kUser))
    user = options.get(mysqlshdk::db::kUser);

  // we need different handling for file
  if (scheme == k_scheme_name_file) {
    if (user.has_value()) {
      throw_exception(
          shcore::str_format("URL should not have a user: %s", url.c_str()));
    }
    if (!options.has_value(mysqlshdk::db::kPath)) {
      throw_exception(
          shcore::str_format("URL does not have a path: %s", url.c_str()));
    }
  } else {
    if (!user.has_value()) {
      throw_exception(
          shcore::str_format("URL does not have a user: %s", url.c_str()));
    }
    if (!options.has_value(mysqlshdk::db::kHost) &&
        !options.has_value(mysqlshdk::db::kSocket) &&
        !options.has_value(mysqlshdk::db::kPipe)) {
      throw_exception(
          shcore::str_format("URL does not have a host: %s", url.c_str()));
    }
    if (options.has_value(mysqlshdk::db::kSchema)) {
      throw_exception(
          shcore::str_format("URL should not have a schema: %s", url.c_str()));
    }
  }

  auto tokens = mysqlshdk::db::uri::formats::user_transport();
  if (scheme.has_value()) {
    tokens.set(mysqlshdk::db::uri::Tokens::Scheme);
  }

  return options.as_uri(tokens);
}

std::string to_string(Secret_type type) {
  switch (type) {
    case Secret_type::PASSWORD:
      return k_secret_type_password;

    case Secret_type::GENERIC:
      return k_secret_type_generic;
  }

  throw std::runtime_error{"Unknown secret type"};
}

Secret_type to_secret_type(const std::string &type) {
  if (type == k_secret_type_password) {
    return Secret_type::PASSWORD;
  }

  if (type == k_secret_type_generic) {
    return Secret_type::GENERIC;
  }

  throw_json_error("Unknown secret type: \"" + type + "\"");
  // to silence compiler's complains
  return Secret_type::PASSWORD;
}

std::string to_string(rapidjson::Document *doc) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc->Accept(writer);
  return std::string{buffer.GetString(), buffer.GetSize()};
}

void validate_object(const rapidjson::Value &object) {
  if (!object.IsObject()) {
    throw_json_error("Expected object");
  }
}

std::string required(const rapidjson::Value &object, const char *name) {
  const auto it = object.FindMember(name);

  if (object.MemberEnd() == it) {
    throw_json_error("Missing member: \"" + std::string(name) + "\"");
  }

  if (!it->value.IsString()) {
    throw_json_error("Member \"" + std::string(name) + "\" is not a string");
  }

  return std::string{it->value.GetString(), it->value.GetStringLength()};
}

rapidjson::Document parse(const std::string &json) {
  rapidjson::Document doc;

  doc.Parse(json.c_str());

  if (doc.HasParseError()) {
    throw_json_error(rapidjson::GetParseError_En(doc.GetParseError()));
  }

  return doc;
}

rapidjson::Document to_object(Secret_type type) {
  rapidjson::Document doc{rapidjson::Type::kObjectType};
  auto &allocator = doc.GetAllocator();

  doc.AddMember(rapidjson::StringRef(k_secret_type),
                {to_string(type).c_str(), allocator}, allocator);

  return doc;
}

rapidjson::Document to_object(const Secret_spec &spec) {
  const auto &spec_id =
      Secret_type::PASSWORD == spec.type ? validate_url(spec.id) : spec.id;

  auto doc = to_object(spec.type);
  auto &allocator = doc.GetAllocator();

  doc.AddMember(rapidjson::StringRef(k_secret_id), {spec_id.c_str(), allocator},
                allocator);
  doc.AddMember(rapidjson::StringRef(k_server_url),
                {spec_id.c_str(), allocator}, allocator);

  return doc;
}

std::string to_json(Secret_type type) {
  auto doc = to_object(type);
  return to_string(&doc);
}

std::string to_json(const Secret_spec &spec, const std::string &secret) {
  auto doc = to_object(spec);
  auto &allocator = doc.GetAllocator();

  doc.AddMember(rapidjson::StringRef(k_secret),
                rapidjson::StringRef(secret.c_str(), secret.length()),
                allocator);

  return to_string(&doc);
}

std::string to_json(const Secret_spec &spec) {
  auto doc = to_object(spec);
  return to_string(&doc);
}

Secret_spec to_secret_spec(const rapidjson::Value &spec) {
  validate_object(spec);

  std::string id;

  // we first check for the deprecated key, to have a meaningful message if the
  // new key is missing
  try {
    id = required(spec, k_server_url);
  } catch (...) {
    id = required(spec, k_secret_id);
  }

  return {to_secret_type(required(spec, k_secret_type)), std::move(id)};
}

void to_secret_spec_list(const std::string &spec,
                         std::vector<Secret_spec> *list) {
  const auto doc = parse(spec);

  if (!doc.IsArray()) {
    throw_json_error("Expected array");
  }

  for (const auto &s : doc.GetArray()) {
    list->emplace_back(to_secret_spec(s));
  }
}

std::pair<Secret_spec, std::string> to_secret(const std::string &secret) {
  const auto doc = parse(secret);

  validate_object(doc);

  return std::make_pair(to_secret_spec(doc), required(doc, k_secret));
}

}  // namespace

class Helper_interface::Helper_interface_impl {
 public:
  explicit Helper_interface_impl(const Helper_name &name) noexcept
      : m_invoker{name} {}

  Helper_name name() const noexcept { return m_invoker.name(); }

  bool store(const Secret_spec &spec, const std::string &secret) noexcept {
    try {
      std::string output;
      bool ret = m_invoker.store(to_json(spec, secret), &output);

      if (ret) {
        clear_last_error();
      } else {
        set_last_error(output);
      }

      return ret;
    } catch (const std::exception &ex) {
      set_last_error(ex.what());
      return false;
    }
  }

  bool get(const Secret_spec &spec, std::string *secret) const noexcept {
    if (nullptr == secret) {
      set_last_error("Invalid pointer");
      return false;
    }

    try {
      std::string output;
      bool ret = m_invoker.get(to_json(spec), &output);

      if (ret) {
        *secret = to_secret(output).second;
        clear_last_error();
      } else {
        set_last_error(output);
      }

      return ret;
    } catch (const std::exception &ex) {
      set_last_error(ex.what());
      return false;
    }
  }

  bool erase(const Secret_spec &spec) noexcept {
    try {
      std::string output;
      bool ret = m_invoker.erase(to_json(spec), &output);

      if (ret) {
        clear_last_error();
      } else {
        set_last_error(output);
      }

      return ret;
    } catch (const std::exception &ex) {
      set_last_error(ex.what());
      return false;
    }
  }

  bool list(std::vector<Secret_spec> *specs,
            std::optional<Secret_type> type = {}) const noexcept {
    if (nullptr == specs) {
      set_last_error("Invalid pointer");
      return false;
    }

    try {
      std::string input;

      if (type.has_value()) {
        input = to_json(*type);
      }

      std::string output;
      bool ret = m_invoker.list(&output, input);

      if (ret) {
        to_secret_spec_list(output, specs);
        clear_last_error();
      } else {
        set_last_error(output);
      }

      return ret;
    } catch (const std::exception &ex) {
      set_last_error(ex.what());
      return false;
    }
  }

  std::string get_last_error() const noexcept { return m_last_error; }

 private:
  void set_last_error(const std::string &str) const { m_last_error = str; }

  void clear_last_error() const { m_last_error.clear(); }

  Helper_invoker m_invoker;
  mutable std::string m_last_error;
};

Helper_interface::Helper_interface(const Helper_name &name) noexcept
    : m_impl{new Helper_interface_impl{name}} {}

Helper_interface::~Helper_interface() noexcept = default;

Helper_name Helper_interface::name() const noexcept { return m_impl->name(); }

bool Helper_interface::store(const Secret_spec &spec,
                             const std::string &secret) noexcept {
  return m_impl->store(spec, secret);
}

bool Helper_interface::get(const Secret_spec &spec,
                           std::string *secret) const noexcept {
  return m_impl->get(spec, secret);
}

bool Helper_interface::erase(const Secret_spec &spec) noexcept {
  return m_impl->erase(spec);
}

bool Helper_interface::list(std::vector<Secret_spec> *specs,
                            std::optional<Secret_type> type) const noexcept {
  return m_impl->list(specs, type);
}

std::string Helper_interface::get_last_error() const noexcept {
  return m_impl->get_last_error();
}

}  // namespace api
}  // namespace secret_store
}  // namespace mysql
