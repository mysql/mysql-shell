/*
 * Copyright (c) 2018, 2024, Oracle and/or its affiliates.
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
#include "unittest/test_utils.h"

#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/preconditions.h"
#include "mysqlshdk/include/scripting/types.h"
#include "mysqlshdk/libs/utils/version.h"
#include "unittest/test_utils/mocks/modules/adminapi/common/mock_metadata_storage.h"
#include "unittest/test_utils/mocks/modules/adminapi/common/mock_precondition_checker.h"
#include "unittest/test_utils/mocks/mysqlshdk/libs/db/mock_session.h"

namespace mysqlsh {
namespace dba {

using testing::_;

using testing::Return;

class Admin_api_preconditions : public Shell_core_test_wrapper {
 protected:
  void SetUp() {
    Shell_core_test_wrapper::SetUp();

    bool m_primary_available = true;
    m_mock_session = std::make_shared<testing::Mock_session>();
    m_mock_instance = std::make_shared<mysqlsh::dba::Instance>(m_mock_session);
    m_mock_session->expect_query(
        {"SELECT COALESCE(@@report_host, @@hostname),  "
         "COALESCE(@@report_port, @@port)",
         {"Host", "Port"},
         {mysqlshdk::db::Type::String, mysqlshdk::db::Type::Integer},
         {{"mock@localhost", "3306"}}});

    m_mock_metadata =
        std::make_shared<testing::Mock_metadata_storage>(m_mock_instance);
    m_preconditions = std::make_shared<mysqlsh::dba::Precondition_checker>(
        m_mock_metadata, m_mock_instance, m_primary_available);
  }

  void TearDown() { Shell_core_test_wrapper::TearDown(); }

  std::shared_ptr<testing::Mock_session> m_mock_session;
  std::shared_ptr<mysqlsh::dba::Instance> m_mock_instance;
  // std::shared_ptr<mysqlsh::dba::MetadataStorage> m_metadata;
  std::shared_ptr<mysqlsh::dba::Precondition_checker> m_preconditions;
  std::shared_ptr<testing::Mock_metadata_storage> m_mock_metadata;
};

TEST_F(Admin_api_preconditions, check_session) {
  // Test inexistent session
  try {
    EXPECT_CALL(*m_mock_metadata, get_md_server())
        .WillOnce(Return(std::make_shared<mysqlsh::dba::Instance>()));
    m_preconditions->check_session();
    SCOPED_TRACE("Unexpected success calling validate_session");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ("An open session is required to perform this operation",
                 e.what());
  }

  // Test closed session
  EXPECT_CALL(*m_mock_metadata, get_md_server());
  EXPECT_CALL(*m_mock_session, is_open()).WillOnce(Return(false));

  try {
    m_preconditions->check_session();
    SCOPED_TRACE("Unexpected success calling validate_session");
    ADD_FAILURE();
  } catch (const shcore::Exception &e) {
    EXPECT_STREQ(
        "The session was closed. An open session is required to perform this "
        "operation",
        e.what());
  }

  mysqlshdk::db::Connection_options coptions("localhost");

  // Test invalid server version
  // Test different invalid server versions
  std::vector<std::string> min_invalid_versions = {"5.6", "5.1", "5.6.99"};

  std::vector<std::string> max_invalid_versions = {
      shcore::str_format("%d.0",
                         mysqlshdk::utils::k_shell_version.get_major() + 1),
      shcore::str_format("%d.%d.0",
                         mysqlshdk::utils::k_shell_version.get_major(),
                         mysqlshdk::utils::k_shell_version.get_minor() + 1),
      shcore::str_format("%d.%d", mysqlshdk::utils::k_shell_version.get_major(),
                         mysqlshdk::utils::k_shell_version.get_minor() + 2)};

  EXPECT_CALL(*m_mock_metadata, get_md_server())
      .Times(min_invalid_versions.size());
  EXPECT_CALL(*m_mock_session, is_open()).Times(min_invalid_versions.size());

  for (const auto &version : min_invalid_versions) {
    EXPECT_CALL(*m_mock_session, get_server_version())
        .WillOnce(Return(mysqlshdk::utils::Version(version)));
    try {
      m_preconditions->check_session();
      SCOPED_TRACE("Unexpected success calling validate_session");
      SCOPED_TRACE(version);
      ADD_FAILURE();
    } catch (const shcore::Exception &e) {
      EXPECT_STREQ(
          "Unsupported server version: AdminAPI operations in this version of "
          "MySQL Shell are supported on MySQL Server 8.0 and above",
          e.what());
    }
  }

  EXPECT_CALL(*m_mock_metadata, get_md_server())
      .Times(max_invalid_versions.size());
  EXPECT_CALL(*m_mock_session, is_open()).Times(max_invalid_versions.size());

  for (const auto &version : max_invalid_versions) {
    EXPECT_CALL(*m_mock_session, get_server_version())
        .WillOnce(Return(mysqlshdk::utils::Version(version)));
    try {
      m_preconditions->check_session();
      SCOPED_TRACE("Unexpected success calling validate_session");
      SCOPED_TRACE(version);
      ADD_FAILURE();
    } catch (const shcore::Exception &e) {
      EXPECT_STREQ(
          shcore::str_format(
              "Unsupported server version: AdminAPI operations in this version "
              "of MySQL Shell support MySQL Server up to version %d.%d",
              mysqlshdk::utils::k_shell_version.get_major(),
              mysqlshdk::utils::k_shell_version.get_minor())
              .c_str(),
          e.what());
    }
  }

  std::vector<std::string> valid_versions;

  valid_versions.push_back(shcore::str_format(
      "%d.%d.%d", mysqlshdk::utils::k_shell_version.get_major(),
      mysqlshdk::utils::k_shell_version.get_minor(),
      mysqlshdk::utils::k_shell_version.get_patch()));

  // Now inserts corner cases for the different versions starting at 8.0
  // Valid versions start at 8.0
  int lower_major = 8;
  for (auto major = lower_major;
       major <= mysqlshdk::utils::k_shell_version.get_major(); major++) {
    // Valid minor versions end at current minor for the last release or at 6
    // for previous releases
    int max_minor = major < mysqlshdk::utils::k_shell_version.get_major()
                        ? 6
                        : mysqlshdk::utils::k_shell_version.get_minor();
    for (auto minor = 0; minor <= max_minor; minor++) {
      valid_versions.push_back(shcore::str_format("%d.%d", major, minor));
      valid_versions.push_back(shcore::str_format("%d.%d.1", major, minor));
      valid_versions.push_back(shcore::str_format("%d.%d.99", major, minor));
    }
  }
  EXPECT_CALL(*m_mock_metadata, get_md_server()).Times(valid_versions.size());
  EXPECT_CALL(*m_mock_session, is_open()).Times(valid_versions.size());

  for (const auto &version : valid_versions) {
    EXPECT_CALL(*m_mock_session, get_server_version())
        .WillOnce(Return(mysqlshdk::utils::Version(version)));

    try {
      m_preconditions->check_session();
    } catch (const shcore::Exception &e) {
      std::string error = "Unexpected failure calling validate_session: ";
      error += e.what();
      SCOPED_TRACE(error);
      SCOPED_TRACE(version);
      ADD_FAILURE();
    }
  }
}

TEST_F(Admin_api_preconditions,
       check_instance_configuration_preconditions_errors) {
  struct Invalid_types {
    mysqlsh::dba::TargetType::Type instance_type;
    int allowed_types;
    int code;
    std::string error;
  };

  std::vector<Invalid_types> validations = {
      {mysqlsh::dba::TargetType::Unknown, 0, 0,
       "Unable to detect state for instance 'mock@localhost:3306'. Please see "
       "the shell log for more details."},
      {mysqlsh::dba::TargetType::Standalone, 0,
       SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED,
       "This function is not available through a session to a standalone "
       "instance"},
      {mysqlsh::dba::TargetType::StandaloneWithMetadata, 0, 0,
       "This function is not available through a session to a standalone "
       "instance (metadata exists, instance does not belong to that "
       "metadata, and GR is not active)"},
      {mysqlsh::dba::TargetType::StandaloneWithMetadata,
       mysqlsh::dba::TargetType::AsyncReplicaSet, 0,
       "This function is not available through a session to a standalone "
       "instance (metadata exists, instance does not belong to that "
       "metadata)"},
      {mysqlsh::dba::TargetType::StandaloneInMetadata, 0,
       SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE,
       "This function is not available through a session to a standalone "
       "instance (metadata exists, instance belongs to that metadata, but "
       "GR is not active)"},
      {mysqlsh::dba::TargetType::StandaloneInMetadata,
       mysqlsh::dba::TargetType::AsyncReplicaSet,
       SHERR_DBA_BADARG_INSTANCE_NOT_ONLINE,
       "This function is not available through a session to a standalone "
       "instance (metadata exists, instance belongs to that metadata)"},
      {mysqlsh::dba::TargetType::GroupReplication, 0, 0,
       "This function is not available through a session to an instance "
       "belonging to an unmanaged replication group"},
      {mysqlsh::dba::TargetType::InnoDBCluster,
       TargetType::Standalone | TargetType::StandaloneWithMetadata |
           TargetType::GroupReplication,
       SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_CLUSTER,
       "This function is not available through a session to an instance "
       "already in an InnoDB Cluster"},
      {mysqlsh::dba::TargetType::InnoDBCluster,
       mysqlsh::dba::TargetType::InnoDBClusterSet,
       SHERR_DBA_BADARG_INSTANCE_NOT_IN_CLUSTERSET,
       "This function is not available through a session to an instance that "
       "belongs to an InnoDB Cluster that is not a member of an InnoDB "
       "ClusterSet"},
      {mysqlsh::dba::TargetType::AsyncReplicaSet, 0,
       SHERR_DBA_BADARG_INSTANCE_MANAGED_IN_REPLICASET,
       "This function is not available through a session to an instance "
       "that is a member of an InnoDB ReplicaSet"},
      {mysqlsh::dba::TargetType::InnoDBClusterSet, 0,
       SHERR_DBA_CLUSTER_ALREADY_IN_CLUSTERSET,
       "This function is not available through a session to an InnoDB Cluster "
       "that belongs to an InnoDB ClusterSet"},
  };

  Precondition_checker checker(m_mock_metadata, m_mock_instance, true);
  for (const auto &validation : validations) {
    // Validates the errors
    EXPECT_THROW_LIKE(checker.check_instance_configuration_preconditions(
                          validation.instance_type, validation.allowed_types),
                      shcore::Exception, validation.error);

    // Validates the error codes when needed
    if (validation.code != 0) {
      try {
        checker.check_instance_configuration_preconditions(
            validation.instance_type, validation.allowed_types);
      } catch (const shcore::Error &error) {
        SCOPED_TRACE(validation.error);
        SCOPED_TRACE("Unexpected Error Code");
        EXPECT_EQ(validation.code, error.code());
      }
    }
  }
}

TEST_F(Admin_api_preconditions, check_managed_instance_status_preconditions) {
  struct Invalid_states {
    mysqlsh::dba::TargetType::Type instance_type;
    mysqlsh::dba::ReplicationQuorum::State instance_quorum_state;
    bool primary_req;
    std::string_view error;
  };

  // ERRORS
  {
    std::vector<Invalid_states> validations_errors = {
        {mysqlsh::dba::TargetType::Type::Unknown,
         mysqlsh::dba::ReplicationQuorum::State(
             mysqlsh::dba::ReplicationQuorum::States::Normal),
         true,
         "This operation requires a PRIMARY instance available: All reachable "
         "members of Primary Cluster are OFFLINE, but there are some "
         "unreachable "
         "members that could be ONLINE."},

        {mysqlsh::dba::TargetType::Type::InnoDBClusterSet,
         mysqlsh::dba::ReplicationQuorum::State(
             mysqlsh::dba::ReplicationQuorum::States::Normal),
         true,
         "The InnoDB Cluster is part of an InnoDB ClusterSet and its PRIMARY "
         "instance isn't available. Operation is not possible when in that "
         "state: All reachable members of Primary Cluster are OFFLINE, but "
         "there "
         "are some unreachable members that could be ONLINE."},
    };

    testing::Mock_precondition_checker checker(m_mock_metadata, m_mock_instance,
                                               false);

    for (const auto &validation : validations_errors) {
      EXPECT_CALL(checker, get_cluster_global_state())
          .WillOnce(
              Return(std::make_pair(Cluster_global_status::OK,
                                    Cluster_availability::SOME_UNREACHABLE)));

      // Validates the errors
      EXPECT_THROW_LIKE(
          checker.check_managed_instance_status_preconditions(
              validation.instance_type, validation.instance_quorum_state,
              validation.primary_req),
          shcore::Exception, validation.error);
    }
  }

  // ERRORS
  {
    std::vector<Invalid_states> validations_errors = {
        {mysqlsh::dba::TargetType::Type::Unknown,
         mysqlsh::dba::ReplicationQuorum::State(
             mysqlsh::dba::ReplicationQuorum::States::Quorumless),
         true, "There is no quorum to perform the operation"}};

    Precondition_checker checker(m_mock_metadata, m_mock_instance, false);
    for (const auto &validation : validations_errors) {
      // Validates the errors
      EXPECT_THROW_LIKE(
          checker.check_managed_instance_status_preconditions(
              validation.instance_type, validation.instance_quorum_state,
              validation.primary_req),
          shcore::Exception, validation.error);
    }
  }

  // SUCESS
  {
    std::vector<Invalid_states> validations_success = {
        {mysqlsh::dba::TargetType::Type::Unknown,
         mysqlsh::dba::ReplicationQuorum::State(
             mysqlsh::dba::ReplicationQuorum::States::Normal),
         true, ""},

        {mysqlsh::dba::TargetType::Type::Unknown,
         mysqlsh::dba::ReplicationQuorum::State(
             mysqlsh::dba::ReplicationQuorum::States::Quorumless),
         true, ""}};

    Precondition_checker checker(m_mock_metadata, m_mock_instance, true);
    for (const auto &validation : validations_success) {
      // Validates the errors
      EXPECT_NO_THROW(checker.check_managed_instance_status_preconditions(
          mysqlsh::dba::TargetType::Type::Unknown,
          validation.instance_quorum_state, validation.primary_req));
    }
  }
}

TEST_F(Admin_api_preconditions, check_quorum_state_preconditions_errors) {
  struct Invalid_states {
    mysqlsh::dba::ReplicationQuorum::State instance_state;
    mysqlsh::dba::ReplicationQuorum::State allowed_states;
    int code;
    std::string error;
  };

  std::vector<Invalid_states> validations = {
      {mysqlsh::dba::ReplicationQuorum::State(
           mysqlsh::dba::ReplicationQuorum::States::Normal),
       {},
       0,
       "Unable to perform this operation"},
      {mysqlsh::dba::ReplicationQuorum::State(
           mysqlsh::dba::ReplicationQuorum::States::Normal),
       mysqlsh::dba::ReplicationQuorum::State(
           mysqlsh::dba::ReplicationQuorum::States::All_online),
       0, "This operation requires all the cluster members to be ONLINE"},
      {mysqlsh::dba::ReplicationQuorum::State(
           ReplicationQuorum::States::Quorumless),
       {},
       SHERR_DBA_GROUP_HAS_NO_QUORUM,
       "There is no quorum to perform the operation"},
      {mysqlsh::dba::ReplicationQuorum::State(ReplicationQuorum::States::Dead),
       {},
       0,
       "Unable to perform the operation on a dead InnoDB cluster"}};

  Precondition_checker checker(m_mock_metadata, m_mock_instance, true);
  for (const auto &validation : validations) {
    // Validates the errors
    EXPECT_THROW_LIKE(checker.check_quorum_state_preconditions(
                          validation.instance_state, validation.allowed_states),
                      shcore::Exception, validation.error);

    // Validates the error codes when needed
    if (validation.code != 0) {
      try {
        checker.check_quorum_state_preconditions(validation.instance_state,
                                                 validation.allowed_states);
      } catch (const shcore::Error &error) {
        SCOPED_TRACE(validation.error);
        SCOPED_TRACE("Unexpected Error Code");
        EXPECT_EQ(validation.code, error.code());
      }
    }
  }
}

TEST_F(Admin_api_preconditions, check_quorum_state_preconditions) {
  std::vector<mysqlsh::dba::ReplicationQuorum::State> quorum_states{
      mysqlsh::dba::ReplicationQuorum::State(
          mysqlsh::dba::ReplicationQuorum::States::Normal),
      mysqlsh::dba::ReplicationQuorum::State(
          ReplicationQuorum::States::Quorumless),
      mysqlsh::dba::ReplicationQuorum::State(ReplicationQuorum::States::Dead)};

  Precondition_checker checker(m_mock_metadata, m_mock_instance, true);
  for (const auto &precondition : Precondition_checker::s_preconditions) {
    for (auto quorum_state : quorum_states) {
      if (quorum_state.matches_any(precondition.second.cluster_status)) {
        EXPECT_NO_THROW(checker.check_quorum_state_preconditions(
            quorum_state, precondition.second.cluster_status));
        SCOPED_TRACE("Unexpected failure!");
        SCOPED_TRACE(precondition.first);
      } else {
        EXPECT_ANY_THROW(checker.check_quorum_state_preconditions(
            quorum_state, precondition.second.cluster_status));
        SCOPED_TRACE("Unexpected success!");
        SCOPED_TRACE(precondition.first);
      }
    }
  }
}

TEST_F(Admin_api_preconditions, check_cluster_set_preconditions_errors) {
  struct Invalid_states {
    mysqlsh::dba::Cluster_global_status_mask allowed_states;
    std::string error;
  };

  testing::Mock_precondition_checker checker(m_mock_metadata, m_mock_instance,
                                             true);

  std::pair<mysqlsh::dba::Cluster_global_status,
            mysqlsh::dba::Cluster_availability>
      global_state{mysqlsh::dba::Cluster_global_status::OK_MISCONFIGURED,
                   mysqlsh::dba::Cluster_availability::ONLINE};

  EXPECT_CALL(checker, get_cluster_global_state())
      .WillOnce(Return(global_state));

  Invalid_states validation = {
      mysqlsh::dba::Cluster_global_status_mask(
          mysqlsh::dba::Cluster_global_status::OK),
      "The InnoDB Cluster is part of an InnoDB ClusterSet and has global "
      "state of " +
          to_string(mysqlsh::dba::Cluster_global_status::OK_MISCONFIGURED) +
          " within the ClusterSet. Operation is not possible when in that "
          "state"};

  EXPECT_THROW_LIKE(
      checker.check_cluster_set_preconditions(validation.allowed_states),
      shcore::Exception, validation.error);

  validation = {
      mysqlsh::dba::Cluster_global_status_mask(
          mysqlsh::dba::Precondition_checker::kClusterGlobalStateAnyOk),
      "The InnoDB Cluster is part of an InnoDB ClusterSet and has global "
      "state of " +
          to_string(mysqlsh::dba::Cluster_global_status::NOT_OK) +
          " within the ClusterSet. Operation is not possible when in that "
          "state"};

  global_state = {mysqlsh::dba::Cluster_global_status::NOT_OK,
                  mysqlsh::dba::Cluster_availability::ONLINE};

  EXPECT_CALL(checker, get_cluster_global_state())
      .WillOnce(Return(global_state));

  EXPECT_THROW_LIKE(
      checker.check_cluster_set_preconditions(validation.allowed_states),
      shcore::Exception, validation.error);

  EXPECT_NO_THROW(checker.check_cluster_set_preconditions(
      mysqlsh::dba::Precondition_checker::kClusterGlobalStateAny));
}

TEST_F(Admin_api_preconditions, check_cluster_set_preconditions) {
  std::vector<std::pair<mysqlsh::dba::Cluster_global_status,
                        mysqlsh::dba::Cluster_availability>>
      global_cluster_states{
          {mysqlsh::dba::Cluster_global_status::OK,
           mysqlsh::dba::Cluster_availability::ONLINE},
          {mysqlsh::dba::Cluster_global_status::OK_MISCONFIGURED,
           mysqlsh::dba::Cluster_availability::ONLINE},
          {mysqlsh::dba::Cluster_global_status::OK_NOT_CONSISTENT,
           mysqlsh::dba::Cluster_availability::ONLINE},
          {mysqlsh::dba::Cluster_global_status::OK_NOT_REPLICATING,
           mysqlsh::dba::Cluster_availability::ONLINE},
          {mysqlsh::dba::Cluster_global_status::UNKNOWN,
           mysqlsh::dba::Cluster_availability::ONLINE},
          {mysqlsh::dba::Cluster_global_status::NOT_OK,
           mysqlsh::dba::Cluster_availability::ONLINE},
          {mysqlsh::dba::Cluster_global_status::INVALIDATED,
           mysqlsh::dba::Cluster_availability::ONLINE}};

  std::set<std::string> cset_exclusive_expected = {
      "Cluster.fenceWrites",          "Cluster.getClusterSet",
      "Cluster.unfenceWrites",        "ClusterSet.createReplicaCluster",
      "ClusterSet.dissolve",          "ClusterSet.execute",
      "ClusterSet.listRouters",       "ClusterSet.options",
      "ClusterSet.removeCluster",     "ClusterSet.routingOptions",
      "ClusterSet.routerOptions",     "ClusterSet.setOption",
      "ClusterSet.setRoutingOption",  "ClusterSet.setupAdminAccount",
      "ClusterSet.setupRouterAccount"};

  std::set<std::string> cset_offline_expected = {
      "ClusterSet.describe",
      "ClusterSet.forcePrimaryCluster",
      "ClusterSet.rejoinCluster",
      "ClusterSet.setPrimaryCluster",
      "ClusterSet.status",
      "Dba.checkInstanceConfiguration",
      "Dba.configureInstance",
      "Dba.getClusterSet",
      "Dba.rebootClusterFromCompleteOutage"};

  std::set<std::string> standalone_cluster_exclusive_expected = {
      "Cluster.createClusterSet", "Cluster.switchToMultiPrimaryMode",
      "Cluster.switchToSinglePrimaryMode"};

  std::set<std::string> cset_always_allowed_expected = {
      "Cluster.describe",       "Cluster.forceQuorumUsingPartitionOf",
      "Cluster.listRouters",    "Cluster.setPrimaryInstance",
      "Cluster.options",        "Cluster.routingOptions",
      "Cluster.routerOptions",  "Cluster.status",
      "Cluster.dissolve",       "Cluster.execute",
      "Dba.getCluster",         "Dba.upgradeMetadata",
      "Dba.dropMetadataSchema", "Cluster.fenceAllTraffic"};

  std::set<std::string> cset_sometimes_allowed_expected = {
      "Cluster.addInstance",
      "Cluster.addReplicaInstance",
      "Cluster.rejoinInstance",
      "Cluster.removeInstance",
      "Cluster.removeRouterMetadata",
      "Cluster.rescan",
      "Cluster.resetRecoveryAccountsPassword",
      "Cluster.setInstanceOption",
      "Cluster.setOption",
      "Cluster.setRoutingOption",
      "Cluster.setupAdminAccount",
      "Cluster.setupRouterAccount"};

  std::set<std::string> cset_exclusive;
  std::set<std::string> cset_offline;
  std::set<std::string> standalone_cluster_exclusive;
  std::set<std::string> cset_always_allowed;
  std::set<std::string> cset_sometimes_allowed;

  testing::Mock_precondition_checker checker(m_mock_metadata, m_mock_instance,
                                             true);
  for (const auto &precondition : Precondition_checker::s_preconditions) {
    // Functions that are exclusive to ClusterSets
    bool cluster_set_exclusive =
        (precondition.second.instance_config_state |
         TargetType::InnoDBClusterSet) == TargetType::InnoDBClusterSet;

    bool cluster_set_offline =
        ((precondition.second.instance_config_state &
          TargetType::InnoDBClusterSet) == TargetType::InnoDBClusterSet) &&
        (precondition.second.instance_config_state &
         TargetType::InnoDBClusterSetOffline);

    // Functions that are forbidden in Clusters that belong to ClusterSets
    bool is_standalone_cluster_exclusive =
        ((precondition.second.instance_config_state &
          TargetType::InnoDBCluster) == TargetType::InnoDBCluster) &&
        !(precondition.second.instance_config_state &
          TargetType::InnoDBClusterSet);

    // Functions allowed in ClusterSet
    bool is_allowed_clusterset =
        precondition.second.instance_config_state & TargetType::InnoDBCluster;

    if (cluster_set_exclusive) {
      cset_exclusive.insert(precondition.first);
      // Check also that the cluster_set_state mask is empty
    } else if (cluster_set_offline) {
      cset_offline.insert(precondition.first);
    } else if (precondition.second.cluster_set_state.empty() &&
               is_standalone_cluster_exclusive) {
      // Functions that are forbidden to cluster sets
      EXPECT_ANY_THROW(checker.check_cluster_set_preconditions(
          precondition.second.cluster_set_state));
      standalone_cluster_exclusive.insert(precondition.first);
    } else if (precondition.second.cluster_set_state ==
               mysqlsh::dba::Cluster_global_status_mask::any()) {
      cset_always_allowed.insert(precondition.first);

      // The global state is not even verified for these functions
      // EXPECT_NO_THROW(checker.check_cluster_set_preconditions(
      //     precondition.second.cluster_set_state));
    } else if (is_allowed_clusterset) {
      cset_sometimes_allowed.insert(precondition.first);
      for (const auto &global_state : global_cluster_states) {
        EXPECT_CALL(checker, get_cluster_global_state())
            .WillRepeatedly(Return(global_state));
        if (precondition.second.cluster_set_state.is_set(global_state.first)) {
          EXPECT_NO_THROW(checker.check_cluster_set_preconditions(
              precondition.second.cluster_set_state));
        } else {
          EXPECT_ANY_THROW(checker.check_cluster_set_preconditions(
              precondition.second.cluster_set_state));
        }
      }
    }
  }
  EXPECT_EQ(cset_exclusive_expected, cset_exclusive);
  EXPECT_EQ(cset_offline_expected, cset_offline);
  EXPECT_EQ(standalone_cluster_exclusive_expected,
            standalone_cluster_exclusive);
  EXPECT_EQ(cset_always_allowed_expected, cset_always_allowed);
  EXPECT_EQ(cset_sometimes_allowed_expected, cset_sometimes_allowed);
}

TEST_F(Admin_api_preconditions, check_cluster_fenced_preconditions) {
  struct Invalid_states {
    bool allowed_on_fenced;
    std::string error;
  };

  testing::Mock_precondition_checker checker(m_mock_metadata, m_mock_instance,
                                             true);

  EXPECT_CALL(checker, get_cluster_status())
      .WillOnce(Return(mysqlsh::dba::Cluster_status::FENCED_WRITES));

  EXPECT_THROW_LIKE(checker.check_cluster_fenced_preconditions(false),
                    shcore::Exception,
                    "Unable to perform the operation on an InnoDB Cluster with "
                    "status FENCED_WRITES");

  EXPECT_NO_THROW(checker.check_cluster_fenced_preconditions(true));

  EXPECT_CALL(checker, get_cluster_status())
      .WillRepeatedly(Return(mysqlsh::dba::Cluster_status::OK));

  EXPECT_NO_THROW(checker.check_cluster_fenced_preconditions(true));

  EXPECT_NO_THROW(checker.check_cluster_fenced_preconditions(false));
}
}  // namespace dba
}  // namespace mysqlsh
