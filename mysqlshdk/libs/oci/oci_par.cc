/*
 * Copyright (c) 2022, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/oci/oci_par.h"

#include <memory>
#include <regex>
#include <utility>

#include "mysqlshdk/libs/storage/backend/http.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace oci {

namespace {

const std::string k_at_manifest_json = "@.manifest.json";

const std::regex k_full_par_parser(
    R"(^https:\/\/objectstorage\.([^\.]+)\.([^\/]+)\/p\/(.+)\/n\/(.+)\/b\/(.*)\/o\/((.*)\/)?(.*)$)");

namespace par_tokens {

const size_t REGION = 1;
const size_t FULL_DOMAIN = 2;
const size_t PARID = 3;
const size_t NAMESPACE = 4;
const size_t BUCKET = 5;
const size_t PREFIX = 6;
[[maybe_unused]] const size_t DIRNAME = 7;
const size_t BASENAME = 8;

}  // namespace par_tokens

}  // namespace

std::string to_string(PAR_access_type access_type) {
  std::string access_type_str;

  switch (access_type) {
    case PAR_access_type::OBJECT_READ:
      access_type_str = "ObjectRead";
      break;
    case PAR_access_type::OBJECT_WRITE:
      access_type_str = "ObjectWrite";
      break;
    case PAR_access_type::OBJECT_READ_WRITE:
      access_type_str = "ObjectReadWrite";
      break;
    case PAR_access_type::ANY_OBJECT_READ:
      access_type_str = "AnyObjectRead";
      break;
    case PAR_access_type::ANY_OBJECT_WRITE:
      access_type_str = "AnyObjectWrite";
      break;
    case PAR_access_type::ANY_OBJECT_READ_WRITE:
      access_type_str = "AnyObjectReadWrite";
      break;
  }

  return access_type_str;
}

std::string to_string(PAR_list_action list_action) {
  std::string par_list_action_str;
  switch (list_action) {
    case PAR_list_action::DENY:
      par_list_action_str = "Deny";
      break;
    case PAR_list_action::LIST_OBJECTS:
      par_list_action_str = "ListObjects";
      break;
  }

  return par_list_action_str;
}

std::string to_string(PAR_type type) {
  switch (type) {
    case PAR_type::MANIFEST:
      return "manifest";

    case PAR_type::PREFIX:
      return "prefix";

    case PAR_type::GENERAL:
      return "general";

    case PAR_type::NONE:
      return "none";
  }

  throw std::logic_error("Should not happen");
}

std::string hide_par_secret(const std::string &par, std::size_t start_at) {
  const auto p = par.find("/p/", start_at);
  // if p is std::string::npos, this will simply return std::string::npos
  const auto n = par.find("/n/", p);

  if (std::string::npos == p || std::string::npos == n) {
    throw std::logic_error("This is not a PAR: " + par);
  }

  return par.substr(0, p + 3) + "<secret>" + par.substr(n);
}

std::string PAR_structure::par_url() const {
  return shcore::str_format("%s/p/%s/n/%s/b/%s/o/", endpoint().c_str(),
                            par_id.c_str(), ns_name.c_str(), bucket.c_str());
}

std::string PAR_structure::object_path() const {
  return shcore::str_format("/p/%s/n/%s/b/%s/o/%s%s", par_id.c_str(),
                            ns_name.c_str(), bucket.c_str(),
                            object_prefix.c_str(), object_name.c_str());
}

std::string PAR_structure::endpoint() const {
  return shcore::str_format("https://objectstorage.%s.%s", region.c_str(),
                            domain.c_str());
}

PAR_type parse_par(const std::string &url, PAR_structure *data) {
  PAR_type ret_val = PAR_type::NONE;
  std::smatch results;

  if (std::regex_match(url, results, k_full_par_parser)) {
    std::string object_name = results[par_tokens::BASENAME];

    if (object_name.empty()) {
      ret_val = PAR_type::PREFIX;
    } else if (object_name == k_at_manifest_json) {
      ret_val = PAR_type::MANIFEST;
    } else {
      // Any other PAR
      ret_val = PAR_type::GENERAL;
    }

    if (data) {
      data->full_url = url;
      data->region = results[par_tokens::REGION];
      data->domain = results[par_tokens::FULL_DOMAIN];
      data->par_id = results[par_tokens::PARID];
      data->ns_name = results[par_tokens::NAMESPACE];
      data->bucket = results[par_tokens::BUCKET];
      data->object_prefix = results[par_tokens::PREFIX];
      data->object_name = std::move(object_name);
    }
  }

  if (data) {
    data->m_type = ret_val;
  }

  return ret_val;
}

std::string IPAR_config::describe_self() const {
  return "OCI " + to_string(m_par.type()) +
         " PAR=" + anonymize_par(m_par.object_path()).masked();
}

std::string IPAR_config::describe_url(const std::string &) const {
  return "prefix='" + m_par.object_prefix + "'";
}

std::unique_ptr<storage::IFile> General_par_config::file(
    const std::string &path) const {
  validate_url(path);

  return std::make_unique<storage::backend::Http_object>(anonymize_par(path),
                                                         true);
}

std::unique_ptr<storage::IDirectory> General_par_config::directory(
    const std::string &) const {
  throw std::invalid_argument(
      "Invalid PAR, expected: "
      "https://objectstorage.<region>.oraclecloud.com/p/<secret>/n/"
      "<namespace>/b/<bucket>/o/[<prefix>/][@.manifest.json]");
}

}  // namespace oci
}  // namespace mysqlshdk
