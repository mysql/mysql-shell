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

#include "unittest/mysql-secret-store-plaintext/plaintext_helper.h"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <algorithm>
#include <utility>

#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_path.h"

using mysql::secret_store::common::Helper_exception_code;
using mysql::secret_store::common::get_helper_exception;

namespace mysql {
namespace secret_store {
namespace plaintext {

namespace {

constexpr auto k_secret = "Secret";
constexpr auto k_secret_type = "SecretType";
constexpr auto k_server_url = "ServerURL";

std::string to_string(rapidjson::Document *doc) {
  rapidjson::StringBuffer buffer;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
  doc->Accept(writer);
  return std::string{buffer.GetString(), buffer.GetSize()};
}

rapidjson::Document parse(const std::string &json) {
  rapidjson::Document doc;

  doc.Parse(json.c_str());

  if (doc.HasParseError()) {
    throw std::runtime_error{std::string{"Failed to parse JSON: "} +
                             rapidjson::GetParseError_En(doc.GetParseError())};
  }

  return doc;
}

std::vector<common::Secret> load_file(const std::string &path) {
  shcore::check_file_writable_or_throw(path);

  std::vector<common::Secret> secrets;

  if (shcore::file_exists(path)) {
    auto doc = parse(shcore::get_text_file(path));

    if (doc.IsArray()) {
      for (const auto &s : doc.GetArray()) {
        secrets.emplace_back(s[k_secret].GetString(),
                             common::Secret_id{s[k_secret_type].GetString(),
                                               s[k_server_url].GetString()});
      }
    }
  }

  return secrets;
}

void save_file(const std::string &path,
               const std::vector<common::Secret> &secrets) {
  shcore::check_file_writable_or_throw(path);

  rapidjson::Document doc{rapidjson::Type::kArrayType};
  auto &allocator = doc.GetAllocator();

  for (const auto &s : secrets) {
    rapidjson::Value v{rapidjson::Type::kObjectType};
    v.AddMember(rapidjson::StringRef(k_secret),
                rapidjson::StringRef(s.secret.c_str(), s.secret.length()),
                allocator);
    v.AddMember(rapidjson::StringRef(k_secret_type),
                rapidjson::StringRef(s.id.secret_type.c_str(),
                                     s.id.secret_type.length()),
                allocator);
    v.AddMember(rapidjson::StringRef(k_server_url),
                rapidjson::StringRef(s.id.url.c_str(), s.id.url.length()),
                allocator);
    doc.PushBack(v, allocator);
  }

  if (!shcore::create_file(path, to_string(&doc))) {
    throw std::runtime_error("Failed to open " + path + " for writing.");
  }
}

std::vector<common::Secret>::iterator find_secret(
    const common::Secret_id &id, std::vector<common::Secret> *secrets) {
  return std::find_if(
      secrets->begin(), secrets->end(), [&id](const common::Secret &s) {
        return s.id.secret_type == id.secret_type && s.id.url == id.url;
      });
}

}  // namespace

Plaintext_helper::Plaintext_helper()
    : common::Helper("plaintext", "0.0.1", MYSH_HELPER_COPYRIGHT) {}

void Plaintext_helper::check_requirements() {}

void Plaintext_helper::store(const common::Secret &secret) {
  auto path = get_file_name();
  auto data = load_file(path);
  auto old_secret = find_secret(secret.id, &data);

  if (old_secret == data.end()) {
    data.emplace_back(secret);
  } else {
    *old_secret = secret;
  }

  save_file(path, data);
}

void Plaintext_helper::get(const common::Secret_id &id, std::string *secret) {
  auto data = load_file(get_file_name());
  auto old_secret = find_secret(id, &data);

  if (old_secret == data.end()) {
    throw get_helper_exception(Helper_exception_code::NO_SUCH_SECRET);
  } else {
    *secret = old_secret->secret;
  }
}

void Plaintext_helper::erase(const common::Secret_id &id) {
  auto path = get_file_name();
  auto data = load_file(path);
  auto old_secret = find_secret(id, &data);

  if (old_secret == data.end()) {
    throw get_helper_exception(Helper_exception_code::NO_SUCH_SECRET);
  } else {
    data.erase(old_secret);
  }

  save_file(path, data);
}

void Plaintext_helper::list(std::vector<common::Secret_id> *secrets) {
  auto data = load_file(get_file_name());

  for (auto &s : data) {
    secrets->emplace_back(std::move(s.id));
  }
}

std::string Plaintext_helper::get_file_name() const {
  return shcore::path::join_path(shcore::get_user_config_path(),
                                 "." + name() + ".json");
}

}  // namespace plaintext
}  // namespace secret_store
}  // namespace mysql
