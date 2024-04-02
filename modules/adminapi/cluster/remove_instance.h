/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_CLUSTER_REMOVE_INSTANCE_H_
#define MODULES_ADMINAPI_CLUSTER_REMOVE_INSTANCE_H_

#include <memory>
#include <string>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/common/replication_account.h"

namespace mysqlsh::dba::cluster {

class Remove_instance {
 public:
  Remove_instance() = delete;

  Remove_instance(Cluster_impl *cluster, Connection_options instance_def,
                  cluster::Remove_instance_options options)
      : m_cluster_impl(cluster),
        m_target_cnx_opts(std::move(instance_def)),
        m_options(std::move(options)),
        m_repl_account_mng{*m_cluster_impl} {
    assert(m_cluster_impl);

    m_target_address =
        m_target_cnx_opts.as_uri(mysqlshdk::db::uri::formats::only_transport());
  };

  Remove_instance(Cluster_impl *cluster,
                  std::shared_ptr<mysqlsh::dba::Instance> target_instance,
                  cluster::Remove_instance_options options)
      : m_cluster_impl(cluster),
        m_target_instance(std::move(target_instance)),
        m_options(std::move(options)),
        m_repl_account_mng{*m_cluster_impl} {
    assert(m_cluster_impl);

    m_target_address = m_target_instance->get_connection_options().as_uri(
        mysqlshdk::db::uri::formats::only_transport());
  };

  Remove_instance(const Remove_instance &) = delete;
  Remove_instance(Remove_instance &&) = delete;
  Remove_instance &operator=(const Remove_instance &) = delete;
  Remove_instance &operator=(Remove_instance &&) = delete;

  ~Remove_instance() = default;

 protected:
  void do_run();

  static constexpr bool supports_undo() noexcept { return false; }

 private:
  void prepare();
  Instance_metadata validate_metadata_for_address() const;

  /**
   * Verify if it is the last instance in the cluster, otherwise it cannot
   * be removed (dissolve must be used instead).
   */
  void ensure_not_last_instance_in_cluster(
      const std::string &removed_uuid) const;

  /**
   * Remove the target instance from metadata.
   *
   * This functions save the instance details (Instance_metadata) at the
   * begining to be able to revert the operation if needed (add it back to the
   * metadata).
   *
   * The operation is performed in a transaction, meaning that the removal is
   * completely performed or nothing is removed if some error occur during the
   * operation.
   *
   * @return an Instance_metadata object with the state information of the
   * removed instance, in order enable this operation to be reverted using this
   * data if needed.
   */
  Instance_metadata remove_instance_metadata() const;

  /**
   * Revert the removal of the instance from the metadata.
   *
   * Re-insert the instance to the metadata using the saved state from the
   * remove_instance_metadata() function.
   *
   * @param instance_def Object with the instance state (definition) to
   * re-insert into the metadata.
   */
  void undo_remove_instance_metadata(
      const Instance_metadata &instance_def) const;

  /**
   * Helper method to prompt the to use the 'force' option if the instance is
   * not available.
   */
  bool prompt_to_force_remove() const;

  void check_protocol_upgrade_possible() const;

  void update_read_replicas_source_for_removed_target(
      const std::string &address) const;

 private:
  Cluster_impl *const m_cluster_impl = nullptr;
  std::string m_target_address;
  std::string m_instance_uuid;
  std::string m_address_in_metadata;
  std::shared_ptr<mysqlsh::dba::Instance> m_target_instance;
  mysqlshdk::db::Connection_options m_target_cnx_opts;
  cluster::Remove_instance_options m_options;
  uint32_t m_instance_id = 0;
  bool m_skip_sync = false;
  Replication_account m_repl_account_mng;
};

}  // namespace mysqlsh::dba::cluster

#endif  // MODULES_ADMINAPI_CLUSTER_REMOVE_INSTANCE_H_
