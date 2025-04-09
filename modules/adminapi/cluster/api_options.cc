/*
 * Copyright (c) 2021, 2025, Oracle and/or its affiliates.
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

#include "modules/adminapi/cluster/api_options.h"

#include <vector>

#include "adminapi/common/api_options.h"
#include "adminapi/common/async_topology.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/common_cmd_options.h"
#include "mysqlshdk/include/shellcore/console.h"

namespace mysqlsh::dba::cluster {

const shcore::Option_pack_def<Add_instance_options>
    &Add_instance_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Add_instance_options> b;

    b.include(&Add_instance_options::recovery_progress);
    b.include(&Add_instance_options::gr_options);
    b.include(&Add_instance_options::clone_options);

    b.optional(kOptionLabel, &Add_instance_options::label);
    b.optional(kOptionCertSubject, &Add_instance_options::cert_subject);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Rejoin_instance_options>
    &Rejoin_instance_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Rejoin_instance_options> b;

    b.include(&Rejoin_instance_options::recovery_progress);
    b.include(&Rejoin_instance_options::gr_options);
    b.include(&Rejoin_instance_options::clone_options);

    b.optional(kOptionTimeout, &Rejoin_instance_options::timeout);
    b.optional(kOptionDryRun, &Rejoin_instance_options::dry_run);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Remove_instance_options>
    &Remove_instance_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Remove_instance_options> b;

    b.optional(kOptionTimeout, &Remove_instance_options::timeout);
    b.optional(kOptionForce, &Remove_instance_options::force);
    b.optional(kOptionDryRun, &Remove_instance_options::dry_run);

    return b.build();
  });

  return opts;
}

const shcore::Option_pack_def<Status_options> &Status_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Status_options> b;

    b.optional(kOptionExtended, &Status_options::extended,
               [](std::string_view name, uint64_t &value) {
                 if (value <= 3) return;
                 throw shcore::Exception::argument_error(shcore::str_format(
                     "Invalid value '%" PRIu64
                     "' for option '%.*s'. It must be an integer in the "
                     "range [0, 3].",
                     value, static_cast<int>(name.length()), name.data()));
               });

    return b.build();
  });

  return opts;
}

namespace {
constexpr shcore::Option_data<bool> kOptionAll{"all"};
}

const shcore::Option_pack_def<Options_options> &Options_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Options_options> b;

    b.optional(kOptionAll, &Options_options::all);

    return b.build();
  });

  return opts;
}

namespace {
constexpr shcore::Option_data<bool> kOptionAddUnmanaged{"addUnmanaged"};
constexpr shcore::Option_data<bool> kOptionRemoveObsolete{"removeObsolete"};
constexpr shcore::Option_data<bool> kOptionUpgradeCommProtocol{
    "upgradeCommProtocol"};
constexpr shcore::Option_data<std::optional<bool>> kOptionUpdateViewChangeUuid{
    "updateViewChangeUuid"};
constexpr shcore::Option_data<std::optional<bool>> kOptionRepairMetadata{
    "repairMetadata"};
}  // namespace

const shcore::Option_pack_def<Rescan_options> &Rescan_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Rescan_options> b;

    b.optional_as<bool>(kOptionAddUnmanaged,
                        [](Rescan_options &options, std::string_view,
                           bool value) { options.auto_add = value; });
    b.optional_as<bool>(kOptionRemoveObsolete,
                        [](Rescan_options &options, std::string_view,
                           bool value) { options.auto_remove = value; });

    b.optional(kOptionUpgradeCommProtocol,
               &Rescan_options::upgrade_comm_protocol);
    b.optional(kOptionUpdateViewChangeUuid,
               &Rescan_options::update_view_change_uuid);
    b.optional(kOptionRepairMetadata, &Rescan_options::repair_metadata);

    return b.build();
  });

  return opts;
}

namespace {
constexpr shcore::Option_data<std::optional<uint32_t>>
    kOptionRunningTransactionsTimeout{"runningTransactionsTimeout"};
}

const shcore::Option_pack_def<Set_primary_instance_options>
    &Set_primary_instance_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Set_primary_instance_options> b;

    b.optional(kOptionRunningTransactionsTimeout,
               &Set_primary_instance_options::running_transactions_timeout);

    return b.build();
  });

  return opts;
}

// Read-Replicas

namespace {
constexpr shcore::Option_data<Replication_sources> kOptionReplicationSources{
    "replicationSources"};
}

const shcore::Option_pack_def<Add_replica_instance_options>
    &Add_replica_instance_options::options() {
  static const auto opts = std::invoke([]() {
    shcore::Option_pack_builder<Add_replica_instance_options> b;

    b.optional(kOptionTimeout, &Add_replica_instance_options::timeout);
    b.optional(kOptionDryRun, &Add_replica_instance_options::dry_run);
    b.optional(kOptionLabel, &Add_replica_instance_options::label);
    b.optional(kOptionCertSubject, &Add_replica_instance_options::cert_subject);

    b.optional_as<shcore::Value>(
        kOptionReplicationSources,
        &Add_replica_instance_options::replication_sources_option,
        [](const shcore::Value &value) -> Replication_sources {
          validate_replication_sources_option(value);

          Replication_sources repl_sources;

          // Iterate the list to build the vector of
          // Managed_async_channel_source(s)
          if (value.get_type() == shcore::Value_type::Array) {
            auto sources =
                value.to_string_container<std::vector<std::string>>();

            repl_sources.replication_sources =
                std::set<Managed_async_channel_source,
                         std::greater<Managed_async_channel_source>>{};

            // The source list is ordered by weight
            int64_t source_weight = k_read_replica_max_weight;

            for (const auto &src : sources) {
              Managed_async_channel_source managed_src_address(src);

              // Add it if not in the list already. The one kept in the list is
              // the one with the highest priority/weight
              if (repl_sources.replication_sources.find(managed_src_address) ==
                  repl_sources.replication_sources.end()) {
                Managed_async_channel_source managed_src(src, source_weight--);
                repl_sources.replication_sources.emplace(
                    std::move(managed_src));
              }
            }

            // Set the source type to CUSTOM
            repl_sources.source_type = Source_type::CUSTOM;
            return repl_sources;
          }

          assert(value.get_type() == shcore::Value_type::String);

          auto string = value.as_string();
          if (shcore::str_caseeq(string, kReplicationSourcesAutoPrimary)) {
            repl_sources.source_type = Source_type::PRIMARY;
          } else if (shcore::str_caseeq(string,
                                        kReplicationSourcesAutoSecondary)) {
            repl_sources.source_type = Source_type::SECONDARY;
          }

          return repl_sources;
        });

    b.include(&Add_replica_instance_options::recovery_progress);
    b.include(&Add_replica_instance_options::clone_options);

    return b.build();
  });

  return opts;
}

}  // namespace mysqlsh::dba::cluster
