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

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string_view>

#include "unittest/gtest_clean.h"
#include "unittest/test_utils.h"
#include "unittest/test_utils/cleanup.h"
#include "unittest/test_utils/command_line_test.h"
#include "unittest/test_utils/test_server.h"

#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_jwt.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_private_key.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "mysqlshdk/libs/oci/instance_principal_credentials_provider.h"
#include "mysqlshdk/libs/oci/resource_principal_credentials_provider.h"

namespace mysqlshdk {
namespace oci {
namespace {

using tests::Cleanup;

class Oci_resource_principal_test : public tests::Command_line_test {
 protected:
  static void SetUpTestCase() {
    s_test_server = std::make_unique<tests::Test_server>();

    if (!s_test_server->start_server(8080, true, false)) {
      // kill the process (if it's there), all test cases will fail with
      // appropriate message
      TearDownTestCase();
    }
  }
  static void TearDownTestCase() {
    if (s_test_server && s_test_server->is_alive()) {
      s_test_server->stop_server();
    }
  }

  static std::string token() {
    Resource_principal_credentials_provider provider;
    provider.initialize();
    EXPECT_EQ(k_region, provider.region());
    EXPECT_EQ(k_tenancy_id, provider.tenancy_id());
    return provider.credentials()->auth_key_id();
  }

  std::string token_using_sdk() {
    EXPECT_EQ(
        0,
        execute(
            {_mysqlsh, "--py", "--execute",
             R"(import oci;print(f'ST${oci.auth.signers.resource_principals_signer.get_resource_principals_signer().get_security_token()}'))",
             nullptr}));
    return shcore::str_strip(_output);
  }

  static std::string jwt() {
    shcore::Jwt token;
    token.header().add("alg", "RS256");
    token.header().add("typ", "JWT");

    token.payload().add("res_tenant", k_tenancy_id);
    token.payload().add(
        "exp", static_cast<uint64_t>(std::chrono::system_clock::to_time_t(
                   Oci_credentials::Clock::now() + std::chrono::minutes(5))));

    return token.header().to_string() + '.' + token.payload().to_string() +
           ".sig";
  }

  static std::string filename() {
    return shcore::get_absolute_path(
        shcore::path::join_path(::getenv("TMPDIR"), random_string()));
  }

  static std::string random_string() {
    return shcore::get_random_string(10, "abcdefghijklmnopqrstuvwxyz");
  }

  static bool is_instance_principal_host() {
    try {
      log_debug("Checking if host supports instance principal authentication");

      Instance_principal_credentials_provider provider;
      provider.initialize();

      log_debug("Host supports instance principal authentication");
      return true;
    } catch (const std::exception &e) {
      log_debug("Host does not support instance principal authentication: %s",
                e.what());
      return false;
    }
  }

  static std::unique_ptr<tests::Test_server> s_test_server;
  static const std::string k_region;
  static const std::string k_tenancy_id;
};

std::unique_ptr<tests::Test_server> Oci_resource_principal_test::s_test_server;

const std::string Oci_resource_principal_test::k_region = "us-ashburn-1";
const std::string Oci_resource_principal_test::k_tenancy_id = "ocid.tenant";

#define FAIL_IF_NO_SERVER                                 \
  do {                                                    \
    if (!s_test_server->is_alive()) {                     \
      FAIL() << "The HTTP test server is not available."; \
    }                                                     \
  } while (false)

#define SKIP_IF_NO_OCI_CONFIGURATION                                         \
  do {                                                                       \
    if (!::getenv("OCI_COMPARTMENT_ID")) {                                   \
      SKIP_TEST("This test is only executed when OCI config is available."); \
    }                                                                        \
  } while (false)

#define SKIP_IF_NOT_INSTANCE_PRINCIPAL                            \
  do {                                                            \
    if (!is_instance_principal_host()) {                          \
      SKIP_TEST("This test is only executed on an OCI compute."); \
    }                                                             \
  } while (false)

TEST_F(Oci_resource_principal_test, no_version) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_VERSION' environment variable is not set");
}

TEST_F(Oci_resource_principal_test, wrong_version) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto cleanup =
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "0.0");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "Unsupported 'OCI_RESOURCE_PRINCIPAL_VERSION': 0.0");
}

TEST_F(Oci_resource_principal_test, version_1_1_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto cleanup =
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "1.1");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT' environment "
                    "variable is not set");
}

TEST_F(Oci_resource_principal_test, version_1_1) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;
  SKIP_IF_NOT_INSTANCE_PRINCIPAL;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "1.1");
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT",
                                  s_test_server->address());

  EXPECT_EQ(token_using_sdk(), token());

  const auto instance_id = random_string();

  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_PATH",
                                  "/20180711/resourcePrincipalTokenV2/{id}");
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_ID", instance_id);

  EXPECT_EQ(token_using_sdk(), token());
}

TEST_F(Oci_resource_principal_test, version_2_1_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.1");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT' environment "
                    "variable is not set");

  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT", "rpt");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT' environment "
                    "variable is not set");

  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT", "rpst");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_REGION' environment variable is not set");

  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", "region");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RESOURCE_ID' environment "
                    "variable is not set");

  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RESOURCE_ID", "resource");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM' environment "
                    "variable is not set");
}

TEST_F(Oci_resource_principal_test, version_2_1) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;

  const auto private_key = shcore::ssl::Private_key::generate();
  const auto resource_id = random_string();

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.1");
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", k_region);
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RESOURCE_ID", resource_id);
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM",
                                  private_key.to_string());

  EXPECT_EQ(token_using_sdk(), token());
}

TEST_F(Oci_resource_principal_test, version_2_1_using_files) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;

  const auto private_key = shcore::ssl::Private_key::generate();
  const auto private_key_path = filename();
  const auto resource_id = random_string();

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.1");
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", k_region);
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RESOURCE_ID", resource_id);
  cleanup += Cleanup::write_file(private_key_path, private_key.to_string());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM",
                                  private_key_path);

  // SDK uses KeyPairSigner to sign the RPT/RPST requests, and that does not
  // support files
  EXPECT_NO_THROW(token());

  const auto pass_phrase = random_string();
  const auto pass_phrase_path = filename();
  private_key.save(pass_phrase_path, pass_phrase);

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_PASSPHRASE", pass_phrase_path);

  EXPECT_NO_THROW(token());
}

TEST_F(Oci_resource_principal_test, version_2_1_1_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup =
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.1.1");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT' environment "
                    "variable is not set");

  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT", "rpt");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT' environment "
                    "variable is not set");

  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT", "rpst");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_REGION' environment variable is not set");

  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", "region");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_TENANCY_ID' environment "
                    "variable is not set");

  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_TENANCY_ID", "tenancy");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RESOURCE_ID' environment "
                    "variable is not set");

  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RESOURCE_ID", "resource");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM' environment "
                    "variable is not set");
}

TEST_F(Oci_resource_principal_test, version_2_1_1) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;

  const auto private_key = shcore::ssl::Private_key::generate();
  const auto resource_id = random_string();

  auto cleanup =
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.1.1");
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", k_region);
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_TENANCY_ID", k_tenancy_id);
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RESOURCE_ID", resource_id);
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM",
                                  private_key.to_string());

  EXPECT_EQ(token_using_sdk(), token());
}

TEST_F(Oci_resource_principal_test, version_2_1_1_using_files) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;

  const auto private_key = shcore::ssl::Private_key::generate();
  const auto private_key_path = filename();
  const auto resource_id = random_string();

  auto cleanup =
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.1.1");
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT",
                                  s_test_server->address());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", k_region);
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_TENANCY_ID", k_tenancy_id);
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RESOURCE_ID", resource_id);
  cleanup += Cleanup::write_file(private_key_path, private_key.to_string());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM",
                                  private_key_path);

  // SDK uses KeyPairSigner to sign the RPT/RPST requests, and that does not
  // support files
  EXPECT_NO_THROW(token());

  const auto pass_phrase = random_string();
  const auto pass_phrase_path = filename();
  private_key.save(pass_phrase_path, pass_phrase);

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_PASSPHRASE", pass_phrase_path);

  EXPECT_NO_THROW(token());
}

TEST_F(Oci_resource_principal_test, version_2_2_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.2");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RPST' environment variable is not set");

  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST", "rpst");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_REGION' environment variable is not set");

  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", "region");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM' environment "
                    "variable is not set");
}

TEST_F(Oci_resource_principal_test, version_2_2) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto rpst = jwt();
  const auto private_key = shcore::ssl::Private_key::generate();

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.2");
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST", rpst);
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", k_region);
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM",
                                  private_key.to_string());

  EXPECT_EQ(token_using_sdk(), token());
}

TEST_F(Oci_resource_principal_test, version_2_2_using_files) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto rpst = jwt();
  const auto rpst_path = filename();
  const auto private_key = shcore::ssl::Private_key::generate();
  const auto private_key_path = filename();

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "2.2");
  cleanup += Cleanup::write_file(rpst_path, rpst);
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPST", rpst_path);
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_REGION", k_region);
  cleanup += Cleanup::write_file(private_key_path, private_key.to_string());
  cleanup += Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM",
                                  private_key_path);

  EXPECT_EQ(token_using_sdk(), token());

  const auto pass_phrase = random_string();
  const auto pass_phrase_path = filename();
  private_key.save(pass_phrase_path, pass_phrase);

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_PASSPHRASE", pass_phrase_path);

  // we cannot use SDK here, because it has a bug, it uses:
  //   open(self.passphrase_file, "b")
  // which is missing the "r" mode and throws
  EXPECT_EQ("ST$" + rpst, token());
}

TEST_F(Oci_resource_principal_test, version_3_0_no_leaf_version) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  const auto cleanup =
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE' "
                    "environment variable is not set");
}

TEST_F(Oci_resource_principal_test, version_3_0_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "1.1");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE' "
                    "environment variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE", "rpt");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE' "
      "environment variable is not set");
}

TEST_F(Oci_resource_principal_test, version_3_0_wrong_leaf_version) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE", "rpt");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE", "rpst");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "0.0");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "Unsupported 'OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE': 0.0");
}

TEST_F(Oci_resource_principal_test,
       version_3_0_leaf_version_1_1_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE", "rpt");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE", "rpst");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "1.1");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE' environment "
      "variable is not set");
}

TEST_F(Oci_resource_principal_test, version_3_0_leaf_version_1_1) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;
  SKIP_IF_NOT_INSTANCE_PRINCIPAL;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE",
                           s_test_server->address() + "/nested_rpt/" +
                               std::to_string(s_test_server->port()));
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE",
      s_test_server->address());

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "1.1");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE",
      s_test_server->address());
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_LEAF_RESOURCE",
      s_test_server->address());

  EXPECT_EQ(token_using_sdk(), token());

  const auto instance_id = random_string();

  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_PATH_FOR_LEAF_RESOURCE",
                           "/20180711/resourcePrincipalTokenV2/{id}");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_ID_FOR_LEAF_RESOURCE", instance_id);

  EXPECT_EQ(token_using_sdk(), token());
}

TEST_F(Oci_resource_principal_test,
       version_3_0_leaf_version_2_1_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE", "rpt");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE", "rpst");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "2.1");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE' environment "
      "variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE", "rpt");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_LEAF_RESOURCE' "
      "environment variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_LEAF_RESOURCE", "rpst");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE' "
                    "environment variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE", "region");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RESOURCE_ID_FOR_LEAF_RESOURCE' environment "
      "variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RESOURCE_ID_FOR_LEAF_RESOURCE", "resource");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE' environment "
      "variable is not set");
}

TEST_F(Oci_resource_principal_test, version_3_0_leaf_version_2_1) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;

  const auto private_key = shcore::ssl::Private_key::generate();
  const auto resource_id = random_string();

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE",
                           s_test_server->address() + "/nested_rpt/" +
                               std::to_string(s_test_server->port()));
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE",
      s_test_server->address());

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "2.1");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE",
      s_test_server->address());
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_LEAF_RESOURCE",
      s_test_server->address());
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE", k_region);
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RESOURCE_ID_FOR_LEAF_RESOURCE", resource_id);
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE",
      private_key.to_string());

  // this doesn't work in SDK at all, when leaf signer is created, it uses the
  // OCI_RESOURCE_PRINCIPAL_VERSION instead of
  // OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE to set the `rp_version`,
  // which causes KeyPairSigner to throw
  EXPECT_NO_THROW(token());

  const auto private_key_path = filename();
  cleanup += Cleanup::write_file(private_key_path, private_key.to_string());
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE", private_key_path);

  EXPECT_NO_THROW(token());

  const auto pass_phrase = random_string();
  const auto pass_phrase_path = filename();
  private_key.save(pass_phrase_path, pass_phrase);

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_PASSPHRASE_FOR_LEAF_RESOURCE",
      pass_phrase_path);

  EXPECT_NO_THROW(token());
}

TEST_F(Oci_resource_principal_test,
       version_3_0_leaf_version_2_1_1_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE", "rpt");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE", "rpst");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "2.1.1");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE' environment "
      "variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE", "rpt");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_LEAF_RESOURCE' "
      "environment variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_LEAF_RESOURCE", "rpst");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE' "
                    "environment variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE", "region");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_TENANCY_ID_FOR_LEAF_RESOURCE' "
                    "environment variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_TENANCY_ID_FOR_LEAF_RESOURCE", "tenancy");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_RESOURCE_ID_FOR_LEAF_RESOURCE' environment "
      "variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RESOURCE_ID_FOR_LEAF_RESOURCE", "resource");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE' environment "
      "variable is not set");
}

TEST_F(Oci_resource_principal_test, version_3_0_leaf_version_2_1_1) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;

  const auto private_key = shcore::ssl::Private_key::generate();
  const auto resource_id = random_string();

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE",
                           s_test_server->address() + "/nested_rpt/" +
                               std::to_string(s_test_server->port()));
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE",
      s_test_server->address());

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "2.1.1");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_ENDPOINT_FOR_LEAF_RESOURCE",
      s_test_server->address());
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_LEAF_RESOURCE",
      s_test_server->address());
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE", k_region);
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_TENANCY_ID_FOR_LEAF_RESOURCE", k_tenancy_id);
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RESOURCE_ID_FOR_LEAF_RESOURCE", resource_id);
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE",
      private_key.to_string());

  // this doesn't work in SDK at all, when leaf signer is created, it uses the
  // OCI_RESOURCE_PRINCIPAL_VERSION instead of
  // OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE to set the `rp_version`,
  // which causes KeyPairSigner to throw
  EXPECT_NO_THROW(token());

  const auto private_key_path = filename();
  cleanup += Cleanup::write_file(private_key_path, private_key.to_string());
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE", private_key_path);

  EXPECT_NO_THROW(token());

  const auto pass_phrase = random_string();
  const auto pass_phrase_path = filename();
  private_key.save(pass_phrase_path, pass_phrase);

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_PASSPHRASE_FOR_LEAF_RESOURCE",
      pass_phrase_path);

  EXPECT_NO_THROW(token());
}

TEST_F(Oci_resource_principal_test,
       version_3_0_leaf_version_2_2_missing_env_vars) {
  SKIP_IF_NO_OCI_CONFIGURATION;

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE", "rpt");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE", "rpst");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "2.2");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_RPST_FOR_LEAF_RESOURCE' "
                    "environment variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_FOR_LEAF_RESOURCE", "rpst");

  EXPECT_THROW_LIKE(Resource_principal_credentials_provider{},
                    std::runtime_error,
                    "The 'OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE' "
                    "environment variable is not set");

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE", "region");

  EXPECT_THROW_LIKE(
      Resource_principal_credentials_provider{}, std::runtime_error,
      "The 'OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE' environment "
      "variable is not set");
}

TEST_F(Oci_resource_principal_test, version_3_0_leaf_version_2_2) {
  SKIP_IF_NO_OCI_CONFIGURATION;
  FAIL_IF_NO_SERVER;

  const auto rpst = jwt();
  const auto private_key = shcore::ssl::Private_key::generate();

  auto cleanup = Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_VERSION", "3.0");
  cleanup +=
      Cleanup::set_env_var("OCI_RESOURCE_PRINCIPAL_RPT_URL_FOR_PARENT_RESOURCE",
                           s_test_server->address() + "/nested_rpt/" +
                               std::to_string(s_test_server->port()));
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_ENDPOINT_FOR_PARENT_RESOURCE",
      s_test_server->address());

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_VERSION_FOR_LEAF_RESOURCE", "2.2");
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_RPST_FOR_LEAF_RESOURCE", rpst);
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_REGION_FOR_LEAF_RESOURCE", k_region);
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE",
      private_key.to_string());

  EXPECT_EQ(token_using_sdk(), token());

  const auto private_key_path = filename();
  cleanup += Cleanup::write_file(private_key_path, private_key.to_string());
  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_FOR_LEAF_RESOURCE", private_key_path);

  EXPECT_EQ(token_using_sdk(), token());

  const auto pass_phrase = random_string();
  const auto pass_phrase_path = filename();
  private_key.save(pass_phrase_path, pass_phrase);

  cleanup += Cleanup::set_env_var(
      "OCI_RESOURCE_PRINCIPAL_PRIVATE_PEM_PASSPHRASE_FOR_LEAF_RESOURCE",
      pass_phrase_path);

  // we cannot use SDK here, because it has a bug, it uses:
  //   open(self.passphrase_file, "b")
  // which is missing the "r" mode and throws
  EXPECT_NO_THROW(token());
}

}  // namespace
}  // namespace oci
}  // namespace mysqlshdk
