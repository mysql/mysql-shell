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

#ifndef MODULES_ADMINAPI_MOD_DBA_ROUTING_GUIDELINE_H_
#define MODULES_ADMINAPI_MOD_DBA_ROUTING_GUIDELINE_H_

#include <string>

#include "modules/adminapi/common/routing_guideline_impl.h"

#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/routing_guideline_options.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "modules/mod_shell_result.h"
#include "mysql/instance.h"
#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_json.h"

namespace mysqlsh::dba {

/**
 * \ingroup AdminAPI
 * $(ROUTINGGUIDELINE_BRIEF)
 *
 * $(ROUTINGGUIDELINE)
 */
class RoutingGuideline : public std::enable_shared_from_this<RoutingGuideline>,
                         public shcore::Cpp_object_bridge {
 public:
#if DOXYGEN_JS
  String name;  //!< $(ROUTINGGUIDELINE_GETNAME_BRIEF)
  String getName();
  Dictionary asJson();
  ShellResult destinations();
  ShellResult routes();
  Undefined addDestination(String name, String match, Dictionary options);
  Undefined addRoute(String name, String match, Array destinations,
                     Dictionary options);
  Undefined removeDestination(String name);
  Undefined removeRoute(String name);
  Undefined show(Dictionary options);
  Undefined rename(String name);
  Undefined setDestinationOption(String destinationName, String option,
                                 String value);
  Undefined setRouteOption(String routeName, String option, String value);
  RoutingGuideline copy(String name);
  Undefined export(String filePath);
#elif DOXYGEN_PY
  str name;  //!< $(ROUTINGGUIDELINE_GETNAME_BRIEF)
  str get_name();
  dict as_json();
  ShellResult destinations();
  ShellResult routes();
  None add_destination(str name, str match, dict options);
  None add_route(str name, str match, list destinations, dict options);
  None remove_destination(str name);
  None remove_route(str name);
  None show(dict options);
  None rename(str name);
  None set_destination_option(str destinationName, str option, str value);
  None set_route_option(str routeName, str option, str value);
  RoutingGuideline copy(str name);
  None export(str filePath);
#endif

  explicit RoutingGuideline(
      const std::shared_ptr<Routing_guideline_impl> &impl);
  virtual ~RoutingGuideline();

  std::string class_name() const override { return "RoutingGuideline"; }
  virtual shcore::Value get_member(const std::string &prop) const override;
  std::string &append_descr(std::string &s_out, int indent = -1,
                            int quote_strings = 0) const override;
  void append_json(shcore::JSON_dumper &dumper) const override;

  virtual bool operator==(const Object_bridge &other) const override;

 public:
  const std::shared_ptr<Routing_guideline_impl> &impl() const { return m_impl; }

  shcore::Dictionary_t as_json() const;
  std::shared_ptr<ShellResult> destinations() const;
  std::shared_ptr<ShellResult> routes() const;
  void add_destination(
      const std::string &name, const std::string &match,
      const shcore::Option_pack_ref<Add_destination_options> &options);
  void add_route(
      const std::string &name, const std::string &source_matcher,
      const shcore::Array_t &destinations,
      const shcore::Option_pack_ref<Add_route_options> &options = {});
  void remove_destination(const std::string &name);
  void remove_route(const std::string &name);
  void show(const shcore::Option_pack_ref<Show_options> &options = {}) const;
  void rename(const std::string &name);
  void set_destination_option(const std::string &destination_name,
                              const std::string &option,
                              const shcore::Value &value);
  void set_route_option(const std::string &route_name,
                        const std::string &option, const shcore::Value &value);
  std::shared_ptr<RoutingGuideline> copy(const std::string &name);
  void export_to_file(const std::string &file_path);

 protected:
  void init();

 private:
  void assert_valid(const std::string &function_name) const;

  template <typename TCallback>
  auto execute_with_pool(TCallback &&f, bool interactive = false) const {
    std::shared_ptr<MetadataStorage> metadata_storage;
    const mysqlshdk::mysql::Auth_options *admin_credentials = nullptr;

    // Helper function to handle owner type logic
    auto get_creds_and_check_md_store = [&](auto owner_ptr) {
      admin_credentials = &owner_ptr->default_admin_credentials();

      if (!owner_ptr->get_metadata_storage()->is_valid()) {
        owner_ptr->set_target_server(owner_ptr->get_cluster_server());
      }
    };

    // Determine the owner type to get the credentials and reset the metadata
    // storage to m_cluster_server if needed. This is needed because the
    // Routing Guideline object might be pointing to an old reference of the
    // metadata storage (failover scenarios for example).
    //
    // NOTE: m_cluster_server is guaranteed to be reachable: checked before in
    // assert_valid()
    if (const auto &cluster =
            std::dynamic_pointer_cast<Cluster_impl>(impl()->owner())) {
      get_creds_and_check_md_store(cluster);
    } else if (const auto &clusterset =
                   std::dynamic_pointer_cast<Cluster_set_impl>(
                       impl()->owner())) {
      get_creds_and_check_md_store(clusterset);
    } else if (const auto &replicaset =
                   std::dynamic_pointer_cast<Replica_set_impl>(
                       impl()->owner())) {
      get_creds_and_check_md_store(replicaset);
    } else {
      throw std::logic_error("Invalid owner type for Routing Guideline.");
    }

    // Initialize the Scoped_instance_pool
    Scoped_instance_pool scoped_pool(interactive, *admin_credentials);

    // Execute the callback function with the initialized pool
    return f();
  }

 private:
  std::shared_ptr<Routing_guideline_impl> m_impl;
};

}  // namespace mysqlsh::dba

#endif  // MODULES_ADMINAPI_MOD_DBA_ROUTING_GUIDELINE_H_
