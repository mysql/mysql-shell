/*
 * Copyright (c) 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <set>
#include <vector>

#include "mysql-secret-store/include/mysql-secret-store/api.h"
#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/secret-store-api/helper_invoker.h"

namespace mysql {
namespace secret_store {
namespace api {

namespace {

constexpr auto k_secret = "Secret";
constexpr auto k_secret_type = "SecretType";
constexpr auto k_server_url = "ServerURL";

constexpr auto k_secret_type_password = "password";

void throw_json_error(const std::string &msg) {
  throw std::runtime_error{"Failed to parse JSON output: " + msg};
}

std::string validate_url(const std::string &url) {
  using mysqlshdk::db::Connection_options;

  auto throw_exception = [](const std::string &msg) {
    throw std::runtime_error{"Invalid URL: " + msg};
  };

  Connection_options options;
  try {
    options = Connection_options{url};
  } catch (const std::exception &e) {
    throw_exception(e.what());
  }

  if (options.has_scheme()) {
    throw_exception("URL should not have a scheme");
  }
  if (!options.has_user()) {
    throw_exception("URL does not have a user");
  }
  if (!options.has_host() && !options.has_socket()) {
    throw_exception("URL does not have a host");
  }
  if (options.has_schema()) {
    throw_exception("URL should not have a schema");
  }
  if (options.has_password()) {
    throw_exception("URL should not have a password");
  }
  if (std::string::npos != url.find("?")) {
    throw_exception("URL should not have options");
  }

  return options.as_uri(mysqlshdk::db::uri::formats::user_transport());
}

void validate_secret(Secret_type type, const std::string &secret) {
  if (Secret_type::PASSWORD == type) {
    if (std::string::npos != secret.find_first_of("\r\n")) {
      throw std::runtime_error{
          "Unable to store password with a newline character"};
    }
  }
}

std::string to_string(Secret_type type) {
  switch (type) {
    case Secret_type::PASSWORD:
      return k_secret_type_password;
  }

  throw std::runtime_error{"Unknown secret type"};
}

Secret_type to_secret_type(const std::string &type) {
  if (type == k_secret_type_password) {
    return Secret_type::PASSWORD;
  }

  throw_json_error("Unknown secret type: \"" + type + "\"");
  // to silence compiler complains
  return Secret_type::PASSWORD;
}

std::string to_string(rapidjson::Document *doc) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc->Accept(writer);
  return std::string{buffer.GetString(), buffer.GetSize()};
}

void validate_object(const rapidjson::Value &object,
                     const std::set<std::string> &members) {
  if (!object.IsObject()) {
    throw_json_error("Expected object");
  }

  for (const auto &m : members) {
    if (!object.HasMember(m.c_str())) {
      throw_json_error("Missing member: \"" + m + "\"");
    }

    if (!object[m.c_str()].IsString()) {
      throw_json_error("Member \"" + m + "\" is not a string");
    }
  }
}

rapidjson::Document parse(const std::string &json) {
  rapidjson::Document doc;

  doc.Parse(json.c_str());

  if (doc.HasParseError()) {
    throw_json_error(rapidjson::GetParseError_En(doc.GetParseError()));
  }

  return doc;
}

rapidjson::Document to_object(const Secret_spec &spec) {
  rapidjson::Document doc{rapidjson::Type::kObjectType};
  auto &allocator = doc.GetAllocator();

  doc.AddMember(rapidjson::StringRef(k_secret_type),
                {to_string(spec.type).c_str(), allocator}, allocator);
  doc.AddMember(rapidjson::StringRef(k_server_url),
                {validate_url(spec.url).c_str(), allocator}, allocator);

  return doc;
}

std::string to_string(const Secret_spec &spec, const std::string &secret) {
  auto doc = to_object(spec);
  auto &allocator = doc.GetAllocator();

  doc.AddMember(rapidjson::StringRef(k_secret),
                rapidjson::StringRef(secret.c_str(), secret.length()),
                allocator);

  return to_string(&doc);
}

std::string to_string(const Secret_spec &spec) {
  auto doc = to_object(spec);
  return to_string(&doc);
}

Secret_spec to_secret_spec(const rapidjson::Value &spec) {
  validate_object(spec, {k_secret_type, k_server_url});

  return {to_secret_type(spec[k_secret_type].GetString()),
          spec[k_server_url].GetString()};
}

void to_secret_spec_list(const std::string &spec,
                         std::vector<Secret_spec> *list) {
  auto doc = parse(spec);

  if (!doc.IsArray()) {
    throw_json_error("Expected array");
  }

  for (const auto &s : doc.GetArray()) {
    list->emplace_back(to_secret_spec(s));
  }
}

std::pair<Secret_spec, std::string> to_secret(const std::string &secret) {
  auto doc = parse(secret);

  validate_object(doc, {k_secret});

  return std::make_pair(to_secret_spec(doc), doc[k_secret].GetString());
}

}  // namespace

class Helper_interface::Helper_interface_impl {
 public:
  explicit Helper_interface_impl(const Helper_name &name) noexcept
      : m_invoker{name} {}

  Helper_name name() const noexcept { return m_invoker.name(); }

  bool store(const Secret_spec &spec, const std::string &secret) noexcept {
    try {
      validate_secret(spec.type, secret);

      std::string output;
      bool ret = m_invoker.store(to_string(spec, secret), &output);

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
      bool ret = m_invoker.get(to_string(spec), &output);

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
      bool ret = m_invoker.erase(to_string(spec), &output);

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

  bool list(std::vector<Secret_spec> *specs) const noexcept {
    if (nullptr == specs) {
      set_last_error("Invalid pointer");
      return false;
    }

    try {
      std::string output;
      bool ret = m_invoker.list(&output);

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

bool Helper_interface::get(const Secret_spec &spec, std::string *secret) const
    noexcept {
  return m_impl->get(spec, secret);
}

bool Helper_interface::erase(const Secret_spec &spec) noexcept {
  return m_impl->erase(spec);
}

bool Helper_interface::list(std::vector<Secret_spec> *specs) const noexcept {
  return m_impl->list(specs);
}

std::string Helper_interface::get_last_error() const noexcept {
  return m_impl->get_last_error();
}

}  // namespace api
}  // namespace secret_store
}  // namespace mysql
