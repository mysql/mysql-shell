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

#include "modules/adminapi/common/execute.h"

#include <atomic>
#include <functional>
#include <optional>
#include <string_view>
#include <thread>

#include "modules/adminapi/cluster/cluster_impl.h"
#include "modules/adminapi/cluster_set/cluster_set_impl.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/replica_set/replica_set_impl.h"
#include "mysqlshdk/libs/textui/progress.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_os.h"
#include "utils/utils_string.h"

namespace mysqlsh::dba {

namespace {

struct Keyword {
  std::string_view full;
  std::string_view shorthand;

  constexpr bool check(std::string_view keyword) const noexcept {
    return (keyword == full) || (keyword == shorthand);
  }
};

constexpr Keyword k_keywork_all{"all", "a"};
constexpr Keyword k_keywork_primary{"primary", "p"};
constexpr Keyword k_keywork_secondaries{"secondaries", "s"};
constexpr Keyword k_keywork_rr{"read-replicas", "rr"};

constexpr std::chrono::milliseconds k_spinner_interval{300};

class Scheduler {
 public:
  using Task = std::function<void()>;

  ~Scheduler() noexcept { finish(); }

  void spawn_threads(size_t num_threads) {
    if (m_finish) return;

    for (size_t i = 0; i < num_threads; i++) {
      auto t = mysqlsh::spawn_scoped_thread([this, i]() {
        mysqlsh::Mysql_thread mysql_thread;

        auto thread_name = shcore::str_format("execute_%zu", i + 1);
        shcore::set_current_thread_name(thread_name.c_str());

        log_info("Thread '%s' started", thread_name.c_str());
        execute();
        log_info("Thread '%s' finished", thread_name.c_str());
      });

      m_threads.push_back(std::move(t));
    }
  }

  void finish() {
    {
      std::lock_guard l(m_mutex);
      m_finish = true;
    }

    m_cvar_work.notify_all();  // wake everyone up

    {
      std::unique_lock l(m_mutex);
      if (m_num_executions > 0)
        m_cvar_executions_done.wait(l,
                                    [this] { return (m_num_executions == 0); });
    }

    for (auto &t : m_threads) {
      try {
        t.join();
      } catch (const std::system_error &err) {
        if (err.code() != std::errc::no_such_process)
          log_info("Thread error: '%s'", err.what());
      }
    }
    m_threads.clear();
  }

  void post(Task task) {
    {
      std::lock_guard l(m_mutex);

      assert(!m_finish);
      if (m_finish) return;

      m_num_tasks++;
      m_tasks.emplace_back(std::move(task));
    }

    m_cvar_work.notify_one();
  }

  void execute() {
    {
      std::lock_guard l(m_mutex);

      if (m_finish) return;
      ++m_num_executions;
    }

    try {
      while (!m_finish) {
        // wait for work
        {
          std::unique_lock l(m_mutex);
          if (m_finish) break;
          if (m_tasks.empty()) m_cvar_work.wait(l);
        }

        if (m_finish) break;

        // either get a task or go back and wait for work
        Task task;
        {
          std::lock_guard l(m_mutex);
          if (m_tasks.empty()) continue;

          task = std::exchange(m_tasks.front(), {});
          m_tasks.pop_front();
        }

        // execute task
        try {
          task();
          task = nullptr;  // causes task dtor to run now
        } catch (const std::exception &exp) {
          log_error("Error executing task: %s", exp.what());
        }

        // are we done?
        {
          std::lock_guard l(m_mutex);

          assert(m_num_tasks > 0);
          --m_num_tasks;
          if (m_num_tasks == 0) m_cvar_tasks_done.notify_all();
        }
      }
    } catch (const std::exception &exp) {
      log_error("Error while working: %s", exp.what());
    }

    {
      std::lock_guard l(m_mutex);
      --m_num_executions;
    }

    m_cvar_executions_done.notify_all();
  }

  void wait_all() {
    std::unique_lock l(m_mutex);

    if (m_num_tasks > 0)
      m_cvar_tasks_done.wait(l, [this] { return (m_num_tasks == 0); });
  }

 private:
  size_t m_num_tasks{0};
  size_t m_num_executions{0};
  std::atomic<bool> m_finish{false};
  std::condition_variable m_cvar_work;
  std::condition_variable m_cvar_tasks_done;
  std::condition_variable m_cvar_executions_done;
  std::mutex m_mutex;
  std::list<Task> m_tasks;
  std::vector<std::thread> m_threads;
};

shcore::Dictionary_t gen_output_main(std::string_view instance_address,
                                     std::string_view instance_label,
                                     std::string_view instance_version = "") {
  auto jmain = shcore::make_dict();

  {
    auto jinfo = shcore::make_dict();

    (*jinfo)["address"] = shcore::Value(instance_address);

    (*jinfo)["version"] = instance_version.empty()
                              ? shcore::Value::Null()
                              : shcore::Value(instance_version);

    if (!instance_label.empty() && (instance_address != instance_label))
      (*jinfo)["label"] = shcore::Value(instance_label);

    (*jmain)["instance"] = shcore::Value(std::move(jinfo));
  }

  return jmain;
}

shcore::Value gen_output_unreachable(std::string_view instance_address,
                                     std::string_view instance_label) {
  assert(!instance_address.empty());

  auto jmain = gen_output_main(instance_address, instance_label);

  {
    auto jerror = shcore::make_dict();
    (*jerror)["type"] = shcore::Value("mysqlsh");
    (*jerror)["message"] = shcore::Value("Instance isn't reachable.");

    (*jmain)["error"] = shcore::Value(std::move(jerror));
  }

  return shcore::Value(std::move(jmain));
}

shcore::Value gen_output_dry_run(std::string_view instance_address,
                                 std::string_view instance_label) {
  assert(!instance_address.empty());

  auto jmain = gen_output_main(instance_address, instance_label);

  {
    auto jwarnings = shcore::make_array();
    jwarnings->push_back(shcore::Value("dry run execution"));

    (*jmain)["warnings"] = shcore::Value(std::move(jwarnings));
  }

  return shcore::Value(std::move(jmain));
}

shcore::Value gen_output_canceled(std::string_view instance_address,
                                  std::string_view instance_label) {
  assert(!instance_address.empty());

  auto jmain = gen_output_main(instance_address, instance_label);

  {
    auto jerror = shcore::make_dict();
    (*jerror)["type"] = shcore::Value("mysqlsh");
    (*jerror)["message"] = shcore::Value("canceled");

    (*jmain)["error"] = shcore::Value(std::move(jerror));
  }

  return shcore::Value(std::move(jmain));
}

std::shared_ptr<Instance> instance_connect(auto &&cb,
                                           const std::string &address) {
  try {
    return cb();
  } catch (const std::exception &exp) {
    current_console()->print_error(shcore::str_format(
        "Unable to connect to instance '%s': %s", address.c_str(), exp.what()));
    throw shcore::Exception(
        shcore::str_format(
            "Invalid instance specified: unable to connect to instance '%s'",
            address.c_str()),
        SHERR_DBA_BADARG_INSTANCE_UNREACHABLE);
  }
}

shcore::Value execute_query(const mysqlshdk::mysql::IInstance &instance,
                            std::string_view instance_label,
                            const std::string &query) {
  auto jmain = gen_output_main(instance.descr(), instance_label,
                               instance.get_version().get_full());

  auto add_error = [&jmain](std::string_view type, std::string_view msg,
                            std::optional<int> code = std::nullopt) {
    auto jerror = shcore::make_dict();
    (*jerror)["type"] = shcore::Value(type);
    (*jerror)["message"] = shcore::Value(msg);
    if (code.has_value()) (*jerror)["code"] = shcore::Value(*code);

    (*jmain)["error"] = shcore::Value(std::move(jerror));
  };

  try {
    auto res = instance.query(query);

    // get warnings, if any
    if (res->get_warning_count() > 0) {
      auto jwarnings = shcore::make_array();
      while (auto warning = res->fetch_one_warning()) {
        jwarnings->push_back(shcore::Value(warning->msg));
      }
      (*jmain)["warnings"] = shcore::Value(std::move(jwarnings));
    }

    (*jmain)["executionTime"] = shcore::Value(res->get_execution_time());

    auto joutput = shcore::make_array();

    while (res->has_resultset()) {
      auto jresultset = shcore::make_dict();

      auto meta = res->get_metadata();

      // columns
      {
        auto j_cnames = shcore::make_array();
        for (const auto &r : meta) {
          auto name = r.get_column_name();
          if (name.empty()) name = r.get_column_label();

          j_cnames->push_back(shcore::Value(std::move(name)));
        }

        (*jresultset)["columnNames"] = shcore::Value(std::move(j_cnames));
      }

      // rows
      {
        auto j_rows = shcore::make_array();

        while (auto row = res->fetch_one()) {
          assert(row->num_fields() == meta.size());

          auto j_row = shcore::make_array();
          for (size_t cur_index = 0; cur_index < meta.size(); ++cur_index) {
            j_row->push_back(shcore::Value(row->get_as_string(cur_index)));
          }
          j_rows->push_back(shcore::Value(std::move(j_row)));
        }

        (*jresultset)["rows"] = shcore::Value(std::move(j_rows));
      }

      (*joutput).push_back(shcore::Value(std::move(jresultset)));

      try {
        // move to the next result set, taking into account that it can throw if
        // there's an error
        if (!res->next_resultset()) break;
      } catch (const shcore::Error &e) {
        add_error("mysql", e.what(), e.code());
        break;
      }
    }

    (*jmain)["output"] = shcore::Value(std::move(joutput));
    return shcore::Value(std::move(jmain));

  } catch (const shcore::Error &e) {
    add_error("mysql", e.what(), e.code());
    return shcore::Value(std::move(jmain));
  }
}

void cancel_query(const std::string &address,
                  const Connection_options &conn_options, uint64_t conn_id) {
  Connection_options instance_cnx_opts(address);
  instance_cnx_opts.set_login_options_from(conn_options);

  log_debug("Connecting to instance '%s'", address.c_str());
  try {
    auto instance = Instance::connect_raw(instance_cnx_opts);
    log_debug("Successfully connected to instance");

    instance->queryf("KILL CONNECTION ?", conn_id);
  } catch (const std::exception &err) {
    log_debug("Failed to connect to instance: %s", err.what());
    throw;
  }
}

}  // namespace

Execute::Instances_def Execute::convert_to_instances_def(
    const shcore::Value &value, bool to_exclude) {
  // NOTE: the checks performed here are just the more basic ones, regardless
  // of context (i.e.: whether the command is used in a Cluster, ClusterSet or
  // ReplicaSet). Further checks are performed later in gather_instances() and
  // exclude_instances() to avoid duplicating logic that must exist there, here.

  if (value.get_type() == shcore::Value_type::Array) {
    auto values = value.to_string_container<std::vector<std::string>>();
    if (values.empty())
      throw shcore::Exception::argument_error(shcore::str_format(
          "The array of instance addresses in option '%s' cannot be empty.",
          to_exclude ? kExecuteExclude : kExecuteInstances));

    return values;
  }

  if (value.get_type() == shcore::Value_type::String) {
    return shcore::str_lower(value.as_string());
  }

  throw shcore::Exception::argument_error(
      shcore::str_format("The option '%s' only accepts a keyword (string) or a "
                         "list of instance addresses.",
                         to_exclude ? kExecuteExclude : kExecuteInstances));
}

// TODO: create unit-tests for the six gather_instances() and
// exclude_instances(). They're static and don't depend on Execute so they are
// already isolated enough so that they are easy to unit-test.

std::vector<Execute::InstanceData> Execute::gather_instances(
    const Cluster_impl &cluster, const Instances_def &instances_add) {
  assert(!std::holds_alternative<std::monostate>(instances_add));

  if (std::holds_alternative<std::vector<std::string>>(instances_add)) {
    const auto &list = std::get<std::vector<std::string>>(instances_add);

    std::vector<InstanceData> instances;
    instances.reserve(list.size());

    for (const auto &address : list) {
      auto i = instance_connect(
          [&cluster, &address]() {
            return cluster.get_session_to_cluster_instance(address);
          },
          address);

      try {
        auto i_md = cluster.get_metadata_storage()->get_instance_by_uuid(
            i->get_uuid(), cluster.get_id());

        instances.emplace_back(i, std::move(i_md.label));

      } catch (const shcore::Exception &exp) {
        if (exp.code() != SHERR_DBA_MEMBER_METADATA_MISSING) throw;

        current_console()->print_error(shcore::str_format(
            "The instance '%s' doesn't belong to Cluster '%s'.",
            address.c_str(), cluster.get_name().c_str()));
        throw shcore::Exception(
            shcore::str_format("Invalid instance specified: instance '%s' "
                               "doesn't belong to the Cluster",
                               address.c_str()),
            SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED);
      }
    }

    return instances;
  }

  assert(std::holds_alternative<std::string>(instances_add));

  const auto &keyword = std::get<std::string>(instances_add);
  const auto is_multi_master = cluster.get_cluster_topology_type() ==
                               mysqlshdk::gr::Topology_mode::MULTI_PRIMARY;

  if (is_multi_master && k_keywork_secondaries.check(keyword)) {
    current_console()->print_error(shcore::str_format(
        "The keyword '%.*s' isn't valid on a multi primary Cluster.",
        static_cast<int>(k_keywork_secondaries.full.size()),
        k_keywork_secondaries.full.data()));
    throw shcore::Exception::runtime_error(
        "Unsupported keyword on a multi primary topology");
  }

  if (k_keywork_all.check(keyword)) {
    std::vector<InstanceData> instances;

    cluster.execute_in_members(
        [&instances](const std::shared_ptr<Instance> &i,
                     const Cluster_impl::Instance_md_and_gr_member &i_md) {
          instances.emplace_back(i, i_md.first.label);
          return true;
        },
        [&instances](const shcore::Error &,
                     const Cluster_impl::Instance_md_and_gr_member &i_md) {
          instances.emplace_back(i_md.first.address, i_md.first.label);
          return true;
        });

    cluster.execute_in_read_replicas(
        [&instances](const std::shared_ptr<Instance> &i,
                     const Instance_metadata &i_md) {
          instances.emplace_back(i, i_md.label);
          return true;
        },
        [&instances](const shcore::Error &, const Instance_metadata &i_md) {
          instances.emplace_back(i_md.address, i_md.label);
          return true;
        });

    return instances;
  }

  if (is_multi_master && k_keywork_primary.check(keyword)) {
    std::vector<InstanceData> instances;

    cluster.execute_in_members(
        [&instances](const std::shared_ptr<Instance> &i,
                     const Cluster_impl::Instance_md_and_gr_member &i_md) {
          instances.emplace_back(i, i_md.first.label);
          return true;
        },
        [&instances](const shcore::Error &,
                     const Cluster_impl::Instance_md_and_gr_member &i_md) {
          instances.emplace_back(i_md.first.address, i_md.first.label);
          return true;
        });

    return instances;
  }

  if (!is_multi_master && k_keywork_primary.check(keyword)) {
    auto i = cluster.get_cluster_server();
    auto i_md = cluster.get_metadata_storage()->get_instance_by_uuid(
        i->get_uuid(), cluster.get_id());

    std::vector<InstanceData> instances;
    instances.emplace_back(std::move(i), std::move(i_md.label));

    return instances;
  }

  if (k_keywork_secondaries.check(keyword)) {
    std::vector<InstanceData> instances;

    const auto &primary_uuid = cluster.get_cluster_server()->get_uuid();
    cluster.execute_in_members(
        [&instances, &primary_uuid](
            const std::shared_ptr<Instance> &i,
            const Cluster_impl::Instance_md_and_gr_member &i_md) {
          if (i_md.second.uuid == primary_uuid) return true;
          instances.emplace_back(i, i_md.first.label);
          return true;
        },
        [&instances, &primary_uuid](
            const shcore::Error &,
            const Cluster_impl::Instance_md_and_gr_member &i_md) {
          if (i_md.second.uuid == primary_uuid) return true;
          instances.emplace_back(i_md.first.address, i_md.first.label);
          return true;
        });

    return instances;
  }

  if (k_keywork_rr.check(keyword)) {
    std::vector<InstanceData> instances;

    cluster.execute_in_read_replicas(
        [&instances](const std::shared_ptr<Instance> &i,
                     const Instance_metadata &i_md) {
          instances.emplace_back(i, i_md.label);
          return true;
        },
        [&instances](const shcore::Error &, const Instance_metadata &i_md) {
          instances.emplace_back(i_md.address, i_md.label);
          return true;
        });

    return instances;
  }

  throw shcore::Exception::argument_error(
      shcore::str_format("\"%s\" is not a valid keyword for option '%s'.",
                         keyword.c_str(), kExecuteInstances));
}

void Execute::exclude_instances(const Cluster_impl &cluster,
                                std::vector<InstanceData> &instances,
                                const Instances_def &instances_exclude) {
  if (std::holds_alternative<std::monostate>(instances_exclude)) return;

  if (std::holds_alternative<std::vector<std::string>>(instances_exclude)) {
    for (const auto &address :
         std::get<std::vector<std::string>>(instances_exclude)) {
      auto i = instance_connect(
          [&cluster, &address]() {
            return cluster.get_session_to_cluster_instance(address);
          },
          address);

      if (!cluster.is_instance_cluster_member(*i)) {
        current_console()->print_error(shcore::str_format(
            "The instance '%s' doesn't belong to Cluster '%s'.",
            address.c_str(), cluster.get_name().c_str()));
        throw shcore::Exception(
            shcore::str_format("Invalid instance specified: instance '%s' "
                               "doesn't belong to the Cluster",
                               address.c_str()),
            SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED);
      }

      instances.erase(std::remove_if(instances.begin(), instances.end(),
                                     [&i](const auto &candidate) {
                                       return (i->get_uuid() ==
                                               candidate.instance->get_uuid());
                                     }),
                      instances.end());
    }

    return;
  }

  assert(std::holds_alternative<std::string>(instances_exclude));

  const auto &keyword = std::get<std::string>(instances_exclude);

  const auto is_multi_master = cluster.get_cluster_topology_type() ==
                               mysqlshdk::gr::Topology_mode::MULTI_PRIMARY;

  if (is_multi_master && k_keywork_secondaries.check(keyword)) {
    current_console()->print_error(shcore::str_format(
        "The keyword '%.*s' isn't valid on a multi primary Cluster.",
        static_cast<int>(k_keywork_secondaries.full.size()),
        k_keywork_secondaries.full.data()));
    throw shcore::Exception::runtime_error(
        "Unsupported keyword on a multi primary topology");
  }

  if (is_multi_master && k_keywork_primary.check(keyword)) {
    auto cluster_instances = cluster.get_metadata_storage()->get_all_instances(
        cluster.get_id(), false);

    for (const auto &i : cluster_instances) {
      auto it = std::find_if(
          instances.begin(), instances.end(), [&i](const auto &candidate) {
            return (candidate.instance->get_uuid() == i.uuid);
          });
      if (it != instances.end()) instances.erase(it);
    }

    return;
  }

  if (!is_multi_master && k_keywork_primary.check(keyword)) {
    auto it = std::find_if(
        instances.begin(), instances.end(),
        [primary = cluster.get_cluster_server()](const auto &candidate) {
          return (candidate.instance->get_uuid() == primary->get_uuid());
        });
    if (it != instances.end()) instances.erase(it);
    return;
  }

  if (k_keywork_secondaries.check(keyword)) {
    auto cluster_instances = cluster.get_metadata_storage()->get_all_instances(
        cluster.get_id(), false);

    auto primary = cluster.get_cluster_server();
    for (const auto &i : cluster_instances) {
      if (i.uuid == primary->get_uuid()) continue;

      auto it = std::find_if(
          instances.begin(), instances.end(), [&i](const auto &candidate) {
            return (candidate.instance->get_uuid() == i.uuid);
          });
      if (it != instances.end()) instances.erase(it);
    }

    return;
  }

  if (k_keywork_rr.check(keyword)) {
    instances.erase(
        std::remove_if(instances.begin(), instances.end(),
                       [&cluster](const auto &candidate) {
                         return cluster.is_read_replica(candidate.instance);
                       }),
        instances.end());
    return;
  }

  throw shcore::Exception::argument_error(shcore::str_format(
      "The keyword \"%s\" is not a valid keyword for option '%s'.",
      keyword.c_str(), kExecuteExclude));
}

std::vector<Execute::InstanceData> Execute::gather_instances(
    Cluster_set_impl &cset, const Instances_def &instances_add) {
  assert(!std::holds_alternative<std::monostate>(instances_add));

  if (std::holds_alternative<std::vector<std::string>>(instances_add)) {
    const auto &list = std::get<std::vector<std::string>>(instances_add);

    auto instances_md = cset.get_all_instances();

    std::vector<InstanceData> instances;
    instances.reserve(list.size());

    for (const auto &address : list) {
      auto i = instance_connect(
          [&cset, &address]() { return cset.connect_target_instance(address); },
          address);

      auto it = std::find_if(
          instances_md.begin(), instances_md.end(), [i](const auto &md) {
            return (!md.invalidated && (md.uuid == i->get_uuid()));
          });
      if (it == instances_md.end()) {
        current_console()->print_error(
            shcore::str_format("The instance '%s' doesn't belong to any "
                               "Cluster of the ClusterSet '%s'.",
                               address.c_str(), cset.get_name().c_str()));
        throw shcore::Exception(
            shcore::str_format("Invalid instance specified: instance '%s' "
                               "doesn't belong to the ClusterSet",
                               address.c_str()),
            SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED);
      }

      instances.emplace_back(i, it->label);
    }

    return instances;
  }

  assert(std::holds_alternative<std::string>(instances_add));

  const auto &keyword = std::get<std::string>(instances_add);

  if (k_keywork_all.check(keyword) || k_keywork_secondaries.check(keyword) ||
      k_keywork_rr.check(keyword)) {
    std::vector<InstanceData> instances;

    std::list<Cluster_id> unreachable;
    for (const auto &cluster :
         cset.connect_all_clusters(false, &unreachable, true)) {
      auto cinstances = gather_instances(*cluster, instances_add);

      instances.insert(instances.end(),
                       std::make_move_iterator(cinstances.begin()),
                       std::make_move_iterator(cinstances.end()));
    }

    if (!unreachable.empty()) {
      unreachable.sort();
      current_console()->print_info(
          shcore::str_format("The follosing clusters are not reachable: %s",
                             shcore::str_join(unreachable, ", ").c_str()));
    }

    return instances;
  }

  if (k_keywork_primary.check(keyword)) {
    return gather_instances(*cset.get_primary_cluster(), instances_add);
  }

  throw shcore::Exception::argument_error(shcore::str_format(
      "The keyword \"%s\" is not a valid keyword for option '%s'.",
      keyword.c_str(), kExecuteInstances));
}

void Execute::exclude_instances(Cluster_set_impl &cset,
                                std::vector<InstanceData> &instances,
                                const Instances_def &instances_exclude) {
  if (std::holds_alternative<std::monostate>(instances_exclude)) return;

  if (std::holds_alternative<std::vector<std::string>>(instances_exclude)) {
    auto instances_md = cset.get_instances_from_metadata();

    for (const auto &address :
         std::get<std::vector<std::string>>(instances_exclude)) {
      auto i = instance_connect(
          [&cset, &address]() { return cset.connect_target_instance(address); },
          address);

      {
        auto it = std::find_if(
            instances_md.begin(), instances_md.end(), [i](const auto &md) {
              return (!md.invalidated && (md.uuid == i->get_uuid()));
            });
        if (it == instances_md.end()) {
          current_console()->print_error(
              shcore::str_format("The instance '%s' doesn't belong to any "
                                 "Cluster of the ClusterSet '%s'.",
                                 address.c_str(), cset.get_name().c_str()));
          throw shcore::Exception(
              shcore::str_format("Invalid instance specified: instance '%s' "
                                 "doesn't belong to the ClusterSet",
                                 address.c_str()),
              SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED);
        }
      }

      instances.erase(std::remove_if(instances.begin(), instances.end(),
                                     [&i](const auto &candidate) {
                                       return (i->get_uuid() ==
                                               candidate.instance->get_uuid());
                                     }),
                      instances.end());
    }

    return;
  }

  assert(std::holds_alternative<std::string>(instances_exclude));

  const auto &keyword = std::get<std::string>(instances_exclude);

  if (k_keywork_secondaries.check(keyword) || k_keywork_rr.check(keyword)) {
    for (const auto &cluster : cset.connect_all_clusters(false, nullptr, true))
      exclude_instances(*cluster, instances, instances_exclude);

    return;
  }

  if (k_keywork_primary.check(keyword)) {
    exclude_instances(*cset.get_primary_cluster(), instances,
                      instances_exclude);
    return;
  }

  throw shcore::Exception::argument_error(shcore::str_format(
      "The keyword \"%s\" is not a valid keyword for option '%s'.",
      keyword.c_str(), kExecuteExclude));
}

std::vector<Execute::InstanceData> Execute::gather_instances(
    const Replica_set_impl &rset, const Instances_def &instances_add) {
  assert(!std::holds_alternative<std::monostate>(instances_add));

  if (std::holds_alternative<std::vector<std::string>>(instances_add)) {
    const auto &list = std::get<std::vector<std::string>>(instances_add);

    auto instances_md = rset.get_instances_from_metadata();

    std::vector<InstanceData> instances;
    instances.reserve(list.size());

    for (const auto &address : list) {
      auto i = instance_connect(
          [&rset, &address]() {
            return rset.get_session_to_cluster_instance(address);
          },
          address);

      auto it = std::find_if(
          instances_md.begin(), instances_md.end(), [i](const auto &md) {
            return (!md.invalidated && (md.uuid == i->get_uuid()));
          });
      if (it == instances_md.end()) {
        current_console()->print_error(shcore::str_format(
            "The instance '%s' doesn't belong to ReplicaSet '%s'.",
            address.c_str(), rset.get_name().c_str()));
        throw shcore::Exception(
            shcore::str_format("Invalid instance specified: instance '%s' "
                               "doesn't belong to the ReplicaSet",
                               address.c_str()),
            SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED);
      }

      instances.emplace_back(i, it->label);
    }

    return instances;
  }

  assert(std::holds_alternative<std::string>(instances_add));

  const auto &keyword = std::get<std::string>(instances_add);

  if (k_keywork_all.check(keyword) || k_keywork_secondaries.check(keyword)) {
    std::list<Instance_metadata> unreachable;
    auto reachable = rset.connect_all_members(0, !k_keywork_all.check(keyword),
                                              &unreachable, true);

    std::vector<InstanceData> instances;
    std::transform(reachable.cbegin(), reachable.cend(),
                   std::back_inserter(instances),
                   [](const auto &i) { return InstanceData(i, ""); });

    for (auto &i_data : instances) {
      auto i_md = rset.get_metadata_storage()->get_instance_by_uuid(
          i_data.instance->get_uuid(), rset.get_id());

      i_data.label = std::move(i_md.label);
    }

    std::transform(unreachable.cbegin(), unreachable.cend(),
                   std::back_inserter(instances), [](const auto &i_md) {
                     return InstanceData(i_md.address, i_md.label);
                   });

    return instances;
  }

  if (k_keywork_primary.check(keyword)) {
    auto i = rset.get_cluster_server();
    auto i_md = rset.get_metadata_storage()->get_instance_by_uuid(
        i->get_uuid(), rset.get_id());

    std::vector<InstanceData> instances;
    instances.emplace_back(std::move(i), std::move(i_md.label));

    return instances;
  }

  throw shcore::Exception::argument_error(shcore::str_format(
      "The keyword \"%s\" is not a valid keyword for option '%s'.",
      keyword.c_str(), kExecuteInstances));
}

void Execute::exclude_instances(const Replica_set_impl &rset,
                                std::vector<InstanceData> &instances,
                                const Instances_def &instances_exclude) {
  if (std::holds_alternative<std::monostate>(instances_exclude)) return;

  if (std::holds_alternative<std::vector<std::string>>(instances_exclude)) {
    auto instances_md = rset.get_instances_from_metadata();

    for (const auto &address :
         std::get<std::vector<std::string>>(instances_exclude)) {
      auto i = instance_connect(
          [&rset, &address]() {
            return rset.get_session_to_cluster_instance(address);
          },
          address);

      auto it = std::find_if(
          instances_md.begin(), instances_md.end(), [i](const auto &md) {
            return (!md.invalidated && (md.uuid == i->get_uuid()));
          });
      if (it == instances_md.end()) {
        current_console()->print_error(shcore::str_format(
            "The instance '%s' doesn't belong to ReplicaSet '%s'.",
            address.c_str(), rset.get_name().c_str()));
        throw shcore::Exception(
            shcore::str_format("Invalid instance specified: instance '%s' "
                               "doesn't belong to the ReplicaSet",
                               address.c_str()),
            SHERR_DBA_BADARG_INSTANCE_NOT_MANAGED);
      }

      instances.erase(std::remove_if(instances.begin(), instances.end(),
                                     [&i](const auto &candidate) {
                                       return (i->get_uuid() ==
                                               candidate.instance->get_uuid());
                                     }),
                      instances.end());
    }

    return;
  }

  assert(std::holds_alternative<std::string>(instances_exclude));

  const auto &keyword = std::get<std::string>(instances_exclude);

  if (k_keywork_primary.check(keyword)) {
    auto it = std::find_if(
        instances.begin(), instances.end(),
        [primary = rset.get_cluster_server()](const auto &candidate) {
          return (candidate.instance->get_uuid() == primary->get_uuid());
        });
    if (it != instances.end()) instances.erase(it);
    return;
  }

  if (k_keywork_secondaries.check(keyword)) {
    auto reachable = rset.connect_all_members(0, true, nullptr, true);

    for (const auto &i : reachable) {
      auto it = std::find_if(
          instances.begin(), instances.end(), [&i](const auto &candidate) {
            return (candidate.instance->get_uuid() == i->get_uuid());
          });
      if (it != instances.end()) instances.erase(it);
    }

    return;
  }

  throw shcore::Exception::argument_error(shcore::str_format(
      "The keyword \"%s\" is not a valid keyword for option '%s'.",
      keyword.c_str(), kExecuteExclude));
}

shcore::Value Execute::do_run(const std::string &cmd,
                              const Instances_def &instances_add,
                              const Instances_def &instances_exclude,
                              std::chrono::seconds timeout) {
  // calculate target instances
  auto instances = std::visit(
      [&instances_add, &instances_exclude](auto &&arg) {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<
                          T, std::reference_wrapper<const Cluster_impl>>) {
          auto insts = gather_instances(arg, instances_add);
          exclude_instances(arg, insts, instances_exclude);
          return insts;
        } else if constexpr (std::is_same_v<
                                 T, std::reference_wrapper<Cluster_set_impl>>) {
          auto insts = gather_instances(arg, instances_add);
          exclude_instances(arg, insts, instances_exclude);
          return insts;
        } else if constexpr (std::is_same_v<T, std::reference_wrapper<
                                                   const Replica_set_impl>>) {
          auto insts = gather_instances(arg, instances_add);
          exclude_instances(arg, insts, instances_exclude);
          return insts;
        } else {
          throw std::logic_error("Unknown topology type");
        }
      },
      m_topo);

  if (instances.empty()) {
    current_console()->print_info(
        "With the specified combination of parameters and options, the list of "
        "target instances, where the command should be executed, is empty.");
    return {};
  }

  // show the final instance list
  {
    std::vector<std::string> addresses;
    addresses.reserve(instances.size());

    std::transform(instances.cbegin(), instances.cend(),
                   std::back_inserter(addresses), [](const auto &i_data) {
                     if (!i_data.instance)
                       return shcore::str_format("'%s'", i_data.label.c_str());

                     return shcore::str_format(
                         "'%s'", i_data.instance->descr().c_str());
                   });

    // so that the user msg is consistent
    std::sort(addresses.begin(), addresses.end());

    auto console = current_console();
    console->print_info(
        shcore::str_format("Statement will be executed at: %s",
                           shcore::str_join(addresses, ", ").c_str()));
    console->print_info();
  }

  // execute
  std::vector<shcore::Value> instances_output;
  if (m_dry_run) {
    instances_output.reserve(instances.size());

    for (const auto &i_data : instances) {
      if (!i_data.instance)
        instances_output.push_back(
            gen_output_unreachable(i_data.address, i_data.label));
      else
        instances_output.push_back(
            gen_output_dry_run(i_data.instance->descr(), i_data.label));
    }
  } else {
    instances_output = execute_parallel(cmd, instances, timeout);
  }

  // last step is to aggregate all the results into a final array
  auto instances_array = shcore::make_array();
  {
    for (auto &i : instances_output) {
      instances_array->push_back(std::move(i));
    }

    instances_output.clear();
  }

  return shcore::Value(std::move(instances_array));
}

std::vector<shcore::Value> Execute::execute_parallel(
    const std::string &cmd, const std::vector<InstanceData> &instances,
    std::chrono::seconds timeout) const {
  std::vector<shcore::Value> result;
  result.reserve(instances.size());

  auto interactive = current_shell_options()->get().wizards;

  size_t num_threads;
  {
    auto platform_threads =
        static_cast<size_t>(std::thread::hardware_concurrency());

    if (interactive) {
      // NOTE: we don't want to overload the system, but we need, at least, 2
      // threads: one for monitoring (and possibly cancelling queries) and
      // another to do the actual work
      if (platform_threads >= 3) platform_threads--;
      if (platform_threads < 2) platform_threads = 2;

      num_threads = std::min(instances.size() + 1, platform_threads);
    } else {
      // NOTE: we don't want to overload the system, but we need, at least, 1
      // thread: to do the actual work
      if (platform_threads >= 2) platform_threads--;
      if (platform_threads < 1) platform_threads = 1;

      num_threads = std::min(instances.size(), platform_threads);
    }
  }

  log_info("Using %zu threads.", num_threads);

  struct Instance_data {
    std::string uuid;
    std::string address;
    Connection_options conn_options;
    uint64_t conn_id;
  };
  Scheduler scheduler;
  std::atomic<bool> canceled{false};
  std::mutex mutex_instances;
  std::vector<Instance_data> instances_cancel;

  shcore::Interrupt_handler handler([&]() {
    canceled = true;
    return false;
  });

  // post a task just to monitor everything and cancel queries if necessary
  if (interactive) {
    scheduler.post([&]() {
      std::optional<mysqlshdk::textui::Spinny_stick> spinner;
      if (isatty(STDOUT_FILENO))
        spinner = mysqlshdk::textui::Spinny_stick(
            "Executing statement on instances (press Ctrl+C to cancel)...");

      bool cancel_done{false};
      while (true) {
        {
          std::lock_guard l(mutex_instances);
          if (result.size() == instances.size()) break;

          if (canceled && !std::exchange(cancel_done, true)) {
            for (const auto &i : instances_cancel)
              cancel_query(i.address, i.conn_options, i.conn_id);

            instances_cancel.clear();
          }
        }

        if (spinner.has_value()) spinner->update();
        shcore::sleep(k_spinner_interval);
      }

      if (spinner.has_value())
        spinner->done(canceled ? "canceled." : "finished.");
    });
  }

  // post a task for each instance
  {
    Connection_options conn_options;
    conn_options = std::visit(
        [](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<
                            T, std::reference_wrapper<const Cluster_impl>>) {
            return arg.get().get_cluster_server()->get_connection_options();

          } else if constexpr (std::is_same_v<T, std::reference_wrapper<
                                                     Cluster_set_impl>>) {
            return arg.get().get_cluster_server()->get_connection_options();
          } else if constexpr (std::is_same_v<T, std::reference_wrapper<
                                                     const Replica_set_impl>>) {
            return arg.get().get_cluster_server()->get_connection_options();
          } else {
            throw std::logic_error("Unknown topology type");
          }
        },
        m_topo);

    for (const auto &i_data : instances) {
      scheduler.post([&, conn_options]() {
        // just print an error if instance isn't reachable
        if (!i_data.instance) {
          auto output = gen_output_unreachable(i_data.address, i_data.label);

          std::lock_guard l(mutex_instances);
          result.push_back(std::move(output));

          return;
        }

        // reaching this point, we are going to execute the query on the
        // instance

        const auto &i = i_data.instance;

        {
          std::lock_guard l(mutex_instances);

          // store this info in case we need to cancel
          instances_cancel.push_back(
              {.uuid = i->get_uuid(),
               .address = i->get_canonical_address(),
               .conn_options = std::move(conn_options),
               .conn_id = i->get_session()->get_connection_id()});
        }

        if (timeout.count() > 0) {
          i->queryf("SET session lock_wait_timeout = ?",
                    static_cast<int>(timeout.count()));
          i->queryf("SET session max_execution_time = ?",
                    static_cast<int>(timeout.count() * 1000));
        }

        shcore::Value res;
        if (canceled)
          res = gen_output_canceled(i->descr(), i_data.label);
        else
          res = execute_query(i, i_data.label, cmd);

        {
          std::lock_guard l(mutex_instances);
          result.push_back(std::move(res));

          instances_cancel.erase(
              std::remove_if(instances_cancel.begin(), instances_cancel.end(),
                             [&i](const auto &candidate) {
                               return (i->get_uuid() == candidate.uuid);
                             }),
              instances_cancel.end());
        }
      });
    }
  }

  // spawn threads to execute tasks
  scheduler.spawn_threads(num_threads);

  DBUG_EXECUTE_IF("execute_cancel", { canceled = true; });

  scheduler.wait_all();  // waits for current tasks to finish
  scheduler.finish();    // informs scheduler to exit and blocks until it does

  assert(result.size() == instances.size());
  return result;
}

}  // namespace mysqlsh::dba
