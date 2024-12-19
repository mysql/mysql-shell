/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_ADMINAPI_COMMON_ROUTING_GUIDELINE_IMPL_H_
#define MODULES_ADMINAPI_COMMON_ROUTING_GUIDELINE_IMPL_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

#include "modules/adminapi/common/api_options.h"
#include "modules/adminapi/common/cluster_types.h"
#include "modules/adminapi/common/metadata_storage.h"
#include "modules/adminapi/common/routing_guideline_options.h"
#include "mysqlshdk/include/scripting/types.h"

namespace mysqlsh::dba {

enum class Read_only_targets { SECONDARIES, READ_REPLICAS, ALL, NONE };

class Base_cluster_impl;

class Guideline_classifier;

class Routing_guideline_impl {
 public:
  explicit Routing_guideline_impl(
      const std::shared_ptr<Base_cluster_impl> &owner,
      const std::string &name = "");

  static std::shared_ptr<Routing_guideline_impl> create(
      const std::shared_ptr<Base_cluster_impl> &owner, const std::string &name,
      shcore::Dictionary_t json,
      const shcore::Option_pack_ref<Create_routing_guideline_options> &options);

  static std::shared_ptr<Routing_guideline_impl> load(
      const std::shared_ptr<Base_cluster_impl> &owner, const std::string &name);

  static std::shared_ptr<Routing_guideline_impl> load(
      const std::shared_ptr<Base_cluster_impl> &owner,
      const Routing_guideline_metadata &guideline);

  static std::shared_ptr<Routing_guideline_impl> import(
      const std::shared_ptr<Base_cluster_impl> &owner,
      const std::string &file_path,
      const shcore::Option_pack_ref<Import_routing_guideline_options> &options);

  std::shared_ptr<Base_cluster_impl> owner() const { return m_owner; }

  const std::string &get_name() const { return m_name; }
  const std::string &get_id() const { return m_id; }
  void set_id(const std::string &id) { m_id = id; }
  void set_as_default() { m_is_default_guideline = true; }
  void unset_as_default() { m_is_default_guideline = false; }
  const bool &is_default() const { return m_is_default_guideline; }

  shcore::Dictionary_t as_json() const;
  std::unique_ptr<mysqlshdk::db::IResult> get_destinations() const;
  std::unique_ptr<mysqlshdk::db::IResult> get_routes() const;

  void add_destination(const std::string &name, const std::string &match,
                       bool dry_run = false, bool no_save = false);

  void add_route(const std::string &name, const std::string &match_expr,
                 const shcore::Array_t &destinations, bool enabled,
                 bool connection_sharing_allowed,
                 std::optional<uint64_t> order = std::nullopt,
                 bool dry_run = false, bool no_save = false);

  void remove_destination(const std::string &name);

  void remove_route(const std::string &name);

  void show(const shcore::Option_pack_ref<Show_options> &options) const;

  void rename(const std::string &name);

  void set_destination_option(const std::string &destination_name,
                              const std::string &option,
                              const shcore::Value &value);

  void set_route_option(const std::string &route_name,
                        const std::string &option, const shcore::Value &value);

  std::shared_ptr<Routing_guideline_impl> copy(const std::string &name);

  void export_to_file(const std::string &file_path);

 public:
  void parse(const shcore::Dictionary_t &json, bool dry_run = false);
  void save_guideline();
  void upgrade_routing_guideline_to_clusterset(
      const Cluster_set_id &cluster_set_id);
  void downgrade_routing_guideline_to_cluster(const Cluster_id &cluster_id);
  static std::string serialize_value(const shcore::Value &value);
  void validate_or_throw_if_invalid();

 private:
  std::shared_ptr<Guideline_classifier> classifier() const;
  void update_classifier();

  void add_named_object_to_array(const std::string &field,
                                 const shcore::Dictionary_t &object,
                                 bool replace = false,
                                 std::optional<uint64_t> order = std::nullopt);
  shcore::Dictionary_t get_named_object(const std::string &field,
                                        const std::string &name);

  void add_default_destinations(Cluster_type type);
  void add_default_routes(
      Cluster_type type,
      const Read_only_targets Read_only_targets = Read_only_targets::NONE);
  std::set<std::string> get_routes_for_destination(
      const std::string &destination_class) const;
  bool remove_named_object(const std::string &field, const std::string &name);
  std::pair<std::map<std::string, std::set<std::string>>,
            std::vector<std::string>>
  classify_topology(const std::string &router = "") const;
  void ensure_unique_or_reuse(
      const std::shared_ptr<Base_cluster_impl> &base_topology, bool force);
  std::string auto_escape_tags(const std::string &expression) const;
  std::string unescape_tags(const std::string &expression) const;

 private:
  std::string m_id;
  std::shared_ptr<Base_cluster_impl> m_owner;
  std::string m_name;
  mysqlshdk::utils::Version m_version;
  bool m_is_default_guideline = false;

  shcore::Dictionary_t m_guideline_doc;
  mutable std::shared_ptr<Guideline_classifier> m_classifier;
  bool m_is_invalidated = false;
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_COMMON_ROUTING_GUIDELINE_IMPL_H_
