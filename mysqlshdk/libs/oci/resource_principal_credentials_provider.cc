/*
 * Copyright (c) 2024, Oracle and/or its affiliates.
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

#include "mysqlshdk/libs/oci/resource_principal_credentials_provider.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "mysqlshdk/libs/storage/utils.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_json.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/oci/instance_metadata_retriever.h"
#include "mysqlshdk/libs/oci/instance_principal_credentials_provider.h"
#include "mysqlshdk/libs/oci/oci_client.h"
#include "mysqlshdk/libs/oci/oci_regions.h"
#include "mysqlshdk/libs/oci/security_token.h"

namespace mysqlshdk {
namespace oci {

namespace {

using shcore::ssl::Keychain;
using shcore::ssl::Private_key;
using shcore::ssl::Private_key_id;

const char *get_env_or_throw(const char *name) {
  const auto env = shcore::get_env(name);

  if (!env.has_value()) {
    throw std::runtime_error(
        shcore::str_format("The '%s' environment variable is not set", name));
  }

  return *env;
}

// Glossary:
//  - RPT - resource principal token
//  - SPST - service principal session token
//  - RPST - resource principal session token

namespace env {

constexpr auto k_oci_resource_principal_version =
    "OCI_RESOURCE_PRINCIPAL_VERSION";

constexpr auto k_oci_resource_principal_rpt_endpoint =
    "OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT";

constexpr auto k_oci_resource_principal_rpt_path =
    "OCI_RESOURCE_PRINCIPAL_RPT_PATH";

constexpr auto k_oci_resource_principal_rpt_id =
    "OCI_RESOURCE_PRINCIPAL_RPT_ID";

constexpr auto k_oci_resource_principal_rpst_endpoint =
    "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT";

constexpr auto k_oci_resource_principal_rpst = "OCI_RESOURCE_PRINCIPAL_RPST";

constexpr auto k_oci_resource_principal_region =
    "OCI_RESOURCE_PRINCIPAL_REGION";

constexpr auto k_oci_resource_principal_tenancy_id =
    "OCI_RESOURCE_PRINCIPAL_TENANCY_ID";

constexpr auto k_oci_resource_principal_resource_id =
    "OCI_RESOURCE_PRINCIPAL_RESOURCE_ID";

constexpr auto k_oci_resource_principal_private_pem =
    "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM";

constexpr auto k_oci_resource_principal_private_pem_passphrase =
    "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_PASSPHRASE";

namespace leaf {

constexpr auto k_oci_resource_principal_version =
    "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_rpt_endpoint =
    "OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_rpt_path =
    "OCI_RESOURCE_PRINCIPAL_RPT_PATH_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_rpt_id =
    "OCI_RESOURCE_PRINCIPAL_RPT_ID_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_rpst_endpoint =
    "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_rpst =
    "OCI_RESOURCE_PRINCIPAL_RPST_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_region =
    "OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_tenancy_id =
    "OCI_RESOURCE_PRINCIPAL_TENANCY_ID_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_resource_id =
    "OCI_RESOURCE_PRINCIPAL_RESOURCE_ID_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_private_pem =
    "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE";

constexpr auto k_oci_resource_principal_private_pem_passphrase =
    "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_PASSPHRASE_FOR_LEAF_RESOURCE";

}  // namespace leaf

namespace parent {

constexpr auto k_oci_resource_principal_rpt_url =
    "OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE";

constexpr auto k_oci_resource_principal_rpst_endpoint =
    "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE";

}  // namespace parent

}  // namespace env

struct Rpst_request {
  std::string rpt;
  std::string spst;
  std::string public_key{};
};

class Rpt_client final : public Oci_client {
 public:
  Rpt_client(Oci_credentials_provider *provider, std::string endpoint,
             std::string path)
      : Oci_client(provider, std::move(endpoint), "OCI-RPT"),
        m_path(std::move(path)) {
    log_debug("OCI RPT-SPST client: %s", service_endpoint().c_str());
  }

  Rpt_client(const Rpt_client &) = delete;
  Rpt_client(Rpt_client &&) = default;

  Rpt_client &operator=(const Rpt_client &) = delete;
  Rpt_client &operator=(Rpt_client &&) = default;

  ~Rpt_client() = default;

  Rpst_request tokens() {
    const auto json = shcore::json::parse_object_or_throw(get(m_path));
    return {
        shcore::json::required(json, "resourcePrincipalToken", false),
        shcore::json::required(json, "servicePrincipalSessionToken", false),
    };
  }

  const mysqlshdk::rest::Headers &response_headers() {
    return response().headers;
  }

 private:
  std::string m_path;
};

class Rpst_client final : public Oci_client {
 public:
  Rpst_client(Oci_credentials_provider *provider, std::string endpoint)
      : Oci_client(provider, std::move(endpoint), "OCI-RPST") {
    log_debug("OCI RPST client: %s", service_endpoint().c_str());
  }

  Rpst_client(const Rpst_client &) = delete;
  Rpst_client(Rpst_client &&) = default;

  Rpst_client &operator=(const Rpst_client &) = delete;
  Rpst_client &operator=(Rpst_client &&) = default;

  ~Rpst_client() = default;

  Security_token token(const Rpst_request &request) {
    shcore::JSON_dumper json;

    json.start_object();
    json.append("resourcePrincipalToken", request.rpt);
    json.append("servicePrincipalSessionToken", request.spst);
    json.append("sessionPublicKey", request.public_key);
    json.end_object();

    return Security_token::from_json(
        post("/v1/resourcePrincipalSessionToken", json.str()));
  }
};

// Gets tenancy ID from the security token, descendants need to provide region.
class Resource_principal : public Oci_credentials_provider {
 public:
  Resource_principal(const Resource_principal &) = delete;
  Resource_principal(Resource_principal &&) = delete;

  Resource_principal &operator=(const Resource_principal &) = delete;
  Resource_principal &operator=(Resource_principal &&) = delete;

 protected:
  explicit Resource_principal(const char *version)
      : Oci_credentials_provider(
            shcore::str_format("resource principal ver. %s", version)) {}

  inline const Private_key_id &private_key_id() const noexcept {
    return m_private_key_id;
  }

  inline const Private_key &private_key() const noexcept {
    return *m_private_key;
  }

 private:
  Credentials fetch_credentials() override {
    log_debug("Refreshing private key of %s", name().c_str());

    m_private_key_id = refresh_private_key();
    m_private_key = std::make_unique<Private_key>(
        shcore::ssl::Private_key_storage::instance().get(m_private_key_id));

    log_debug("Refreshing RPST of %s", name().c_str());

    m_security_token = refresh_rpst();

    log_debug("Setting up tenancy ID of %s", name().c_str());

    set_tenancy_id(
        std::string{m_security_token.jwt().payload().get_string("res_tenant")});

    return {m_security_token.auth_key_id(), m_private_key_id.id(),
            m_security_token.expiration()};
  }

  virtual const Private_key_id &refresh_private_key() = 0;

  virtual Security_token refresh_rpst() = 0;

  Private_key_id m_private_key_id{{}};
  std::unique_ptr<Private_key> m_private_key;
  Security_token m_security_token;
};

// Descendants need to provide region.
class Resource_principal_with_rpt : public Resource_principal {
 protected:
  Resource_principal_with_rpt(
      std::unique_ptr<Oci_credentials_provider> provider,
      std::string rpt_endpoint, std::string rpt_path, std::string rpst_endpoint,
      const char *version)
      : Resource_principal(version),
        m_provider(std::move(provider)),
        m_rpt_client(m_provider.get(), std::move(rpt_endpoint),
                     std::move(rpt_path)),
        m_rpst_client(m_provider.get(), std::move(rpst_endpoint)) {}

 protected:
  inline Oci_credentials_provider &provider() noexcept { return *m_provider; }

  Security_token refresh_rpst() override {
    if (!m_provider->is_initialized()) {
      m_provider->initialize();
    }

    log_debug("Executing RPT request (%s)", name().c_str());

    auto request = m_rpt_client.tokens();
    request.public_key = private_key().public_key().to_string();

    log_debug("Executing RPST request (%s)", name().c_str());

    return m_rpst_client.token(request);
  }

  std::string rpt_response_header(const std::string &name) {
    const auto &headers = m_rpt_client.response_headers();

    if (const auto it = headers.find(name); headers.end() != it) {
      return it->second;
    } else {
      return {};
    }
  }

 private:
  std::unique_ptr<Oci_credentials_provider> m_provider;
  Rpt_client m_rpt_client;
  Rpst_client m_rpst_client;
};

// Gets region from the instance principal.
class Resource_principal_federation : public Resource_principal_with_rpt {
 public:
  Resource_principal_federation(
      std::unique_ptr<Instance_principal_credentials_provider> provider_,
      std::string rpt_endpoint, std::string rpt_path, std::string rpst_endpoint)
      : Resource_principal_with_rpt(
            std::move(provider_), std::move(rpt_endpoint), std::move(rpt_path),
            std::move(rpst_endpoint), "1.1") {
    if (!provider().is_initialized()) {
      provider().initialize();
    }

    set_region(provider().region());
  }

  static std::unique_ptr<Resource_principal_federation> construct(
      const char *rpt_endpoint_env_var, const char *rpt_path_env_var,
      const char *rpt_id_env_var, const char *rpst_endpoint_env_var) {
    auto rpt_endpoint = get_env_or_throw(rpt_endpoint_env_var);

    auto provider = std::make_unique<Instance_principal_credentials_provider>();
    provider->initialize();
    const auto provider_ptr = provider.get();

    return std::make_unique<Resource_principal_federation>(
        std::move(provider), std::move(rpt_endpoint),
        rpt_path(rpt_path_env_var, rpt_id_env_var),
        rpst_endpoint(rpst_endpoint_env_var, provider_ptr));
  }

 private:
  static std::string rpt_path(const char *path_env_var,
                              const char *id_env_var) {
    const auto path_template =
        shcore::get_env(path_env_var)
            .value_or("/20180711/resourcePrincipalToken/{id}");
    std::string id;

    if (const auto env = shcore::get_env(id_env_var); env.has_value()) {
      id = *env;
    } else {
      // fetch ID from IMDS
      id = Instance_metadata_retriever{}.instance_id();
    }

    return shcore::str_replace(path_template, "{id}", id);
  }

  static std::string rpst_endpoint(const char *endpoint_env_var,
                                   const Oci_credentials_provider *provider) {
    if (const auto env = shcore::get_env(endpoint_env_var); env.has_value()) {
      return *env;
    } else {
      return Oci_client::endpoint_for("auth", provider->region());
    }
  }

  const Private_key_id &refresh_private_key() override {
    m_keychain.use(Private_key::generate());
    return m_keychain.id();
  }

  Keychain m_keychain;
};

class Ephemeral_private_key {
 public:
  Ephemeral_private_key(std::string private_key, std::string pass_phrase)
      : m_private_key_path(std::move(private_key)),
        m_pass_phrase(std::move(pass_phrase)) {
    m_is_file = shcore::path::is_absolute(m_private_key_path);

    if (!m_pass_phrase.empty() &&
        m_is_file != shcore::path::is_absolute(m_pass_phrase)) {
      throw std::runtime_error(
          "Cannot mix path and constant settings for ephemeral private key and "
          "its pass phrase");
    }

    if (!m_is_file) {
      m_keychain.use(Private_key::from_string(
          m_private_key_path, &get_pass_phrase, &m_pass_phrase));
      m_private_key_path.clear();
    }
  }

  Ephemeral_private_key(const Ephemeral_private_key &) = delete;
  Ephemeral_private_key(Ephemeral_private_key &&) = delete;

  Ephemeral_private_key &operator=(const Ephemeral_private_key &) = delete;
  Ephemeral_private_key &operator=(Ephemeral_private_key &&) = delete;

  ~Ephemeral_private_key() { shcore::clear_buffer(m_pass_phrase); }

  const Private_key_id &refresh() {
    if (m_is_file) {
      std::string pass_phrase;
      shcore::on_leave_scope clear_pass_phrase{
          [&pass_phrase]() { shcore::clear_buffer(pass_phrase); }};

      if (!m_pass_phrase.empty()) {
        pass_phrase = shcore::str_strip(shcore::get_text_file(m_pass_phrase));
      }

      m_keychain.use(Private_key::from_file(m_private_key_path,
                                            &get_pass_phrase, &pass_phrase));
    }

    return id();
  }

  inline const Private_key_id &id() const noexcept { return m_keychain.id(); }

 private:
  static int get_pass_phrase(char *buf, int size, int /* rwflag */, void *u) {
    const auto pass = reinterpret_cast<std::string *>(u);
    const auto pass_size = std::min(size, static_cast<int>(pass->size()));

    ::memcpy(buf, pass->data(), pass_size);

    return pass_size;
  }

  std::string m_private_key_path;
  std::string m_pass_phrase;
  bool m_is_file;
  Keychain m_keychain;
};

// Does not provide either region or tenancy ID.
class Ephemeral_private_key_credentials_provider
    : public Oci_credentials_provider {
 public:
  Ephemeral_private_key_credentials_provider(std::string auth_key_id,
                                             const Ephemeral_private_key &key)
      : Oci_credentials_provider("ephemeral private key"), m_key(key) {
    m_credentials.auth_key_id = std::move(auth_key_id);
  }

  Ephemeral_private_key_credentials_provider(
      const Ephemeral_private_key_credentials_provider &) = delete;
  Ephemeral_private_key_credentials_provider(
      Ephemeral_private_key_credentials_provider &&) = delete;

  Ephemeral_private_key_credentials_provider &operator=(
      const Ephemeral_private_key_credentials_provider &) = delete;
  Ephemeral_private_key_credentials_provider &operator=(
      Ephemeral_private_key_credentials_provider &&) = delete;

 private:
  Credentials fetch_credentials() override {
    m_credentials.private_key_id = m_key.id().id();

    return m_credentials;
  }

  const Ephemeral_private_key &m_key;
  Credentials m_credentials;
};

// Gets region from an environment variable.
class Ephemeral_resource_principal_federation
    : public Resource_principal_with_rpt {
 public:
  Ephemeral_resource_principal_federation(std::string rpt_endpoint,
                                          std::string rpst_endpoint,
                                          const std::string &region,
                                          const std::string &resource_id,
                                          std::string private_key,
                                          std::string pass_phrase)
      : Ephemeral_resource_principal_federation(
            std::move(rpt_endpoint), std::move(rpst_endpoint), region,
            resource_id,
            shcore::str_format("resource/v2.1/%s", resource_id.c_str()),
            std::move(private_key), std::move(pass_phrase), "2.1") {}

  Ephemeral_resource_principal_federation(std::string rpt_endpoint,
                                          std::string rpst_endpoint,
                                          const std::string &region,
                                          const std::string &tenancy_id,
                                          const std::string &resource_id,
                                          std::string private_key,
                                          std::string pass_phrase)
      : Ephemeral_resource_principal_federation(
            std::move(rpt_endpoint), std::move(rpst_endpoint), region,
            resource_id,
            shcore::str_format("resource/v2.1.1/%s/%s", tenancy_id.c_str(),
                               resource_id.c_str()),
            std::move(private_key), std::move(pass_phrase), "2.1.1") {}

  static std::unique_ptr<Ephemeral_resource_principal_federation> construct(
      const char *rpt_endpoint_env_var, const char *rpst_endpoint_env_var,
      const char *region_env_var, const char *resource_id_env_var,
      const char *private_key_env_var, const char *pass_pharse_env_var) {
    auto rpt_endpoint = get_env_or_throw(rpt_endpoint_env_var);
    auto rpst_endpoint = get_env_or_throw(rpst_endpoint_env_var);
    auto region = get_env_or_throw(region_env_var);
    auto resource_id = get_env_or_throw(resource_id_env_var);
    auto private_key = get_env_or_throw(private_key_env_var);

    return std::make_unique<Ephemeral_resource_principal_federation>(
        std::move(rpt_endpoint), std::move(rpst_endpoint), std::move(region),
        std::move(resource_id), std::move(private_key),
        shcore::get_env(pass_pharse_env_var).value_or(""));
  }

  static std::unique_ptr<Ephemeral_resource_principal_federation> construct(
      const char *rpt_endpoint_env_var, const char *rpst_endpoint_env_var,
      const char *region_env_var, const char *tenancy_id_env_var,
      const char *resource_id_env_var, const char *private_key_env_var,
      const char *pass_pharse_env_var) {
    auto rpt_endpoint = get_env_or_throw(rpt_endpoint_env_var);
    auto rpst_endpoint = get_env_or_throw(rpst_endpoint_env_var);
    auto region = get_env_or_throw(region_env_var);
    auto tenancy_id = get_env_or_throw(tenancy_id_env_var);
    auto resource_id = get_env_or_throw(resource_id_env_var);
    auto private_key = get_env_or_throw(private_key_env_var);

    return std::make_unique<Ephemeral_resource_principal_federation>(
        std::move(rpt_endpoint), std::move(rpst_endpoint), std::move(region),
        std::move(tenancy_id), std::move(resource_id), std::move(private_key),
        shcore::get_env(pass_pharse_env_var).value_or(""));
  }

 private:
  Ephemeral_resource_principal_federation(
      std::string rpt_endpoint, std::string rpst_endpoint,
      const std::string &region, const std::string &resource_id,
      std::string auth_key_id, std::string private_key, std::string pass_phrase,
      const char *version)
      : Resource_principal_with_rpt(
            std::make_unique<Ephemeral_private_key_credentials_provider>(
                std::move(auth_key_id), m_ephemeral_key),
            std::move(rpt_endpoint),
            shcore::str_format("/20180711/resourcePrincipalTokenV2/%s",
                               resource_id.c_str()),
            std::move(rpst_endpoint), version),
        m_ephemeral_key(std::move(private_key), std::move(pass_phrase)) {
    set_region(std::string{regions::from_short_name(region)});
  }

  const Private_key_id &refresh_private_key() override {
    return m_ephemeral_key.refresh();
  }

  Security_token refresh_rpst() override {
    // refresh private key used to fetch the RPT and RPST
    provider().initialize();
    return Resource_principal_with_rpt::refresh_rpst();
  }

  Ephemeral_private_key m_ephemeral_key;
};

// Gets region from an environment variable.
class Ephemeral_resource_principal : public Resource_principal {
 public:
  Ephemeral_resource_principal(std::string session_token,
                               const std::string &region,
                               std::string private_key, std::string pass_phrase)
      : Resource_principal("2.2"),
        m_ephemeral_key(std::move(private_key), std::move(pass_phrase)),
        m_session_token_path(std::move(session_token)) {
    m_is_file = shcore::path::is_absolute(m_session_token_path);

    set_region(std::string{regions::from_short_name(region)});
  }

  static std::unique_ptr<Ephemeral_resource_principal> construct(
      const char *rpst_env_var, const char *region_env_var,
      const char *private_key_env_var, const char *pass_pharse_env_var) {
    auto rpst = get_env_or_throw(rpst_env_var);
    auto region = get_env_or_throw(region_env_var);
    auto private_key = get_env_or_throw(private_key_env_var);

    return std::make_unique<Ephemeral_resource_principal>(
        std::move(rpst), std::move(region), std::move(private_key),
        shcore::get_env(pass_pharse_env_var).value_or(""));
  }

 private:
  const Private_key_id &refresh_private_key() override {
    return m_ephemeral_key.refresh();
  }

  Security_token refresh_rpst() override {
    if (m_is_file) {
      return Security_token{
          shcore::str_strip(shcore::get_text_file(m_session_token_path))};
    } else {
      return Security_token{m_session_token_path};
    }
  }

  Ephemeral_private_key m_ephemeral_key;
  std::string m_session_token_path;
  bool m_is_file;
  Security_token m_security_token;
};

// Gets region from the provided resource principal.
class Nested_resource_principal : public Resource_principal_with_rpt {
 public:
  Nested_resource_principal(std::unique_ptr<Resource_principal> base_provider,
                            std::string rpt_url, std::string rpst_endpoint)
      : Resource_principal_with_rpt(
            std::move(base_provider), get_uri_endpoint(rpt_url),
            get_uri_path(rpt_url), std::move(rpst_endpoint), "3.0"),
        m_rpt_url(std::move(rpt_url)) {
    if (!provider().is_initialized()) {
      provider().initialize();
    }

    set_region(provider().region());
  }

  inline const std::string &rpt_url() const noexcept { return m_rpt_url; }

  inline const std::string &parent_rpt_url() const noexcept {
    return m_parent_rpt_url;
  }

 private:
  static std::size_t span_uri_host(const std::string &uri) {
    const auto uri_no_scheme = storage::utils::strip_scheme(uri);
    const auto pos = uri_no_scheme.find('/');

    if (std::string::npos == pos) {
      return uri.size();
    } else {
      return uri.size() - uri_no_scheme.size() + pos;
    }
  }

  static std::string get_uri_endpoint(const std::string &uri) {
    return uri.substr(0, span_uri_host(uri));
  }

  static std::string get_uri_path(const std::string &uri) {
    const auto pos = span_uri_host(uri);

    if (pos >= uri.length()) {
      return {};
    } else {
      return uri.substr(pos);
    }
  }

  const Private_key_id &refresh_private_key() override {
    // use key from the base provider
    return provider().credentials()->private_key_id();
  }

  Security_token refresh_rpst() override {
    auto token = Resource_principal_with_rpt::refresh_rpst();
    m_parent_rpt_url = rpt_response_header(s_parent_rpt_url_header);
    return token;
  }

  std::string m_rpt_url;
  std::string m_parent_rpt_url;
  static std::string s_parent_rpt_url_header;
};

std::string Nested_resource_principal::s_parent_rpt_url_header =
    "opc-parent-rpt-url";

}  // namespace

Resource_principal_credentials_provider::
    Resource_principal_credentials_provider()
    : Oci_credentials_provider("resource_principal") {
  const auto version = get_env_or_throw(env::k_oci_resource_principal_version);

  log_debug("Using resource principal version %s", version);

  if (std::string_view("1.1") == version) {
    m_resource_principal = Resource_principal_federation::construct(
        env::k_oci_resource_principal_rpt_endpoint,
        env::k_oci_resource_principal_rpt_path,
        env::k_oci_resource_principal_rpt_id,
        env::k_oci_resource_principal_rpst_endpoint);
  } else if (std::string_view("2.1") == version) {
    m_resource_principal = Ephemeral_resource_principal_federation::construct(
        env::k_oci_resource_principal_rpt_endpoint,
        env::k_oci_resource_principal_rpst_endpoint,
        env::k_oci_resource_principal_region,
        env::k_oci_resource_principal_resource_id,
        env::k_oci_resource_principal_private_pem,
        env::k_oci_resource_principal_private_pem_passphrase);
  } else if (std::string_view("2.1.1") == version) {
    m_resource_principal = Ephemeral_resource_principal_federation::construct(
        env::k_oci_resource_principal_rpt_endpoint,
        env::k_oci_resource_principal_rpst_endpoint,
        env::k_oci_resource_principal_region,
        env::k_oci_resource_principal_tenancy_id,
        env::k_oci_resource_principal_resource_id,
        env::k_oci_resource_principal_private_pem,
        env::k_oci_resource_principal_private_pem_passphrase);
  } else if (std::string_view("2.2") == version) {
    m_resource_principal = Ephemeral_resource_principal::construct(
        env::k_oci_resource_principal_rpst,
        env::k_oci_resource_principal_region,
        env::k_oci_resource_principal_private_pem,
        env::k_oci_resource_principal_private_pem_passphrase);
  } else if (std::string_view("3.0") == version) {
    const auto leaf_version =
        get_env_or_throw(env::leaf::k_oci_resource_principal_version);

    log_debug("Using leaf resource principal version %s", leaf_version);

    const auto rpt_url =
        get_env_or_throw(env::parent::k_oci_resource_principal_rpt_url);
    const auto rpst_endpoint =
        get_env_or_throw(env::parent::k_oci_resource_principal_rpst_endpoint);

    std::unique_ptr<Resource_principal> leaf_resource;

    if (std::string_view("1.1") == leaf_version) {
      leaf_resource = Resource_principal_federation::construct(
          env::leaf::k_oci_resource_principal_rpt_endpoint,
          env::leaf::k_oci_resource_principal_rpt_path,
          env::leaf::k_oci_resource_principal_rpt_id,
          env::leaf::k_oci_resource_principal_rpst_endpoint);
    } else if (std::string_view("2.1") == leaf_version) {
      leaf_resource = Ephemeral_resource_principal_federation::construct(
          env::leaf::k_oci_resource_principal_rpt_endpoint,
          env::leaf::k_oci_resource_principal_rpst_endpoint,
          env::leaf::k_oci_resource_principal_region,
          env::leaf::k_oci_resource_principal_resource_id,
          env::leaf::k_oci_resource_principal_private_pem,
          env::leaf::k_oci_resource_principal_private_pem_passphrase);
    } else if (std::string_view("2.1.1") == leaf_version) {
      leaf_resource = Ephemeral_resource_principal_federation::construct(
          env::leaf::k_oci_resource_principal_rpt_endpoint,
          env::leaf::k_oci_resource_principal_rpst_endpoint,
          env::leaf::k_oci_resource_principal_region,
          env::leaf::k_oci_resource_principal_tenancy_id,
          env::leaf::k_oci_resource_principal_resource_id,
          env::leaf::k_oci_resource_principal_private_pem,
          env::leaf::k_oci_resource_principal_private_pem_passphrase);
    } else if (std::string_view("2.2") == leaf_version) {
      leaf_resource = Ephemeral_resource_principal::construct(
          env::leaf::k_oci_resource_principal_rpst,
          env::leaf::k_oci_resource_principal_region,
          env::leaf::k_oci_resource_principal_private_pem,
          env::leaf::k_oci_resource_principal_private_pem_passphrase);
    } else {
      throw std::runtime_error(shcore::str_format(
          "Unsupported '%s': %s", env::leaf::k_oci_resource_principal_version,
          leaf_version));
    }

    log_debug("Parent RPT URL of nested resource principal: %s", rpt_url);

    auto nested_resource = std::make_unique<Nested_resource_principal>(
        std::move(leaf_resource), rpt_url, rpst_endpoint);
    nested_resource->initialize();
    int depth = 1;

    while (!nested_resource->parent_rpt_url().empty()) {
      if (nested_resource->rpt_url() == nested_resource->parent_rpt_url()) {
        throw std::runtime_error(
            "Nested resource principals: detected a cycle");
      }

      if (++depth > 9) {
        throw std::runtime_error(
            "Nested resource principals: hit recursion limit");
      }

      std::string parent_rpt_url = nested_resource->parent_rpt_url();

      log_debug("Parent RPT URL of nested resource principal: %s (depth: %d)",
                parent_rpt_url.c_str(), depth);

      nested_resource = std::make_unique<Nested_resource_principal>(
          std::move(nested_resource), std::move(parent_rpt_url), rpst_endpoint);
      nested_resource->initialize();
    }

    m_resource_principal = std::move(nested_resource);
  } else {
    throw std::runtime_error(
        shcore::str_format("Unsupported '%s': %s",
                           env::k_oci_resource_principal_version, version));
  }

  if (!m_resource_principal->is_initialized()) {
    m_resource_principal->initialize();
  }

  set_region(m_resource_principal->region());
  set_tenancy_id(m_resource_principal->tenancy_id());
}

Resource_principal_credentials_provider::Credentials
Resource_principal_credentials_provider::fetch_credentials() {
  const auto creds = m_resource_principal->credentials();
  return {creds->auth_key_id(), creds->private_key_id().id(),
          Oci_credentials::Clock::to_time_t(creds->expiration())};
}

}  // namespace oci
}  // namespace mysqlshdk
