/*
 * Copyright (c) 2017, 2025, Oracle and/or its affiliates.
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

#include "modules/util/upgrade_check.h"

#include <algorithm>
#include <cinttypes>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <string>
#include <thread>

#include "modules/util/upgrade_checker/common.h"
#include "modules/util/upgrade_checker/manual_check.h"
#include "modules/util/upgrade_checker/upgrade_check_registry.h"
#include "mysqlshdk/include/shellcore/scoped_contexts.h"
#include "mysqlshdk/include/shellcore/shell_init.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/utils/utils.h"
#include "mysqlshdk/libs/utils/utils_string.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {
namespace {
struct Stats_info {
  void update(const Upgrade_issue::Level level) {
    switch (level) {
      case Upgrade_issue::ERROR:
        errors++;
        break;
      case Upgrade_issue::WARNING:
        warnings++;
        break;
      case Upgrade_issue::NOTICE:
        notices++;
        break;
    }
  }

  int errors = 0;
  int warnings = 0;
  int notices = 0;
};

struct Check_info {
  std::string_view name;
  uint64_t connection_id = 0;
  bool kill_session_required = false;
};

/*
 * IMonitoring_context is a separation interface for the monitoring
 * part of threaded time-out run of Upgrade Checker.
 */
class IMonitoring_context {
 public:
  virtual ~IMonitoring_context() = default;

  virtual bool is_executing() const = 0;

  virtual void set_check_timedout() = 0;

  virtual bool wait() = 0;

  virtual const Check_info &current_check() const = 0;

  virtual void on_exception(std::exception_ptr ex) = 0;

  virtual bool is_interrupted() const = 0;

  virtual void kill_check() = 0;
};

std::shared_ptr<mysqlshdk::db::ISession> clone_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  auto &co = session->get_connection_options();
  std::shared_ptr<mysqlshdk::db::ISession> new_session;
  switch (co.get_session_type()) {
    case mysqlsh::SessionType::X:
      new_session = mysqlshdk::db::mysqlx::Session::create();
      break;

    case mysqlsh::SessionType::Classic:
      new_session = mysqlshdk::db::mysql::Session::create();
      break;

    default:
      throw std::runtime_error(
          "Upgrade Checker Check: Unsupported session type.");
  }

  new_session->connect(co);

  new_session->execute("SET SQL_MODE=DEFAULT");

  return new_session;
}

/*
 * Execution_context class encapsulates threading functionality for
 * Upgrade Checker that is required to properly run the time-out option.
 *
 * When running with the time-out option, Upgrade Checker is run in two
 * separate threads: monitoring (that counts the time) and execution
 * (runs the proper UC checks).
 *
 * To force synchronization between those threads, this class contains
 * a class wide locked mutex, that is only unlocked, when monitoring
 * is waiting for current check to run.
 */
class Execution_context : public IMonitoring_context {
 public:
  explicit Execution_context(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const size_t wait_time_seconds)
      : m_main_session(session) {
    m_execution_active.test_and_set();
    m_interrupt_flag.clear();
    if (wait_time_seconds > 0) {
      m_wait_time = std::chrono::seconds(wait_time_seconds);
      log_debug("Custom check timeout was set: %zu seconds", wait_time_seconds);

      m_execution_lock = std::unique_lock{m_checklist_mutex};
    }
  }

  ~Execution_context() override {
    if (m_kill_session) {
      m_kill_session->close();
    }
  }

  void stop() { m_execution_active.clear(); }

  void wake_up() { m_checklist_condition.notify_one(); }

  const std::exception_ptr exception() const noexcept {
    return m_thread_exception;
  }

  void set_check_timedout() override { m_check_on_time = false; }

  void on_new_check(const Check_info &info) {
    {
      auto lock = std::lock_guard(m_checklist_mutex);
      m_current_check = info;
      m_check_on_time = true;
    }
    m_checklist_condition.notify_one();  // notify monitoring
  }

  bool is_check_timedout(const std::unique_ptr<Upgrade_check> &check,
                         const mysqlshdk::db::Error &ex) const {
    if (!m_wait_time.has_value() || m_check_on_time) {
      return false;
    }
    if (check->is_custom_session_required()) {
      return ex.code() == CR_SERVER_LOST;
    }
    return ex.code() == ER_QUERY_INTERRUPTED;
  }

  bool is_executing() const override {
    return m_execution_active.test() && !m_interrupt_flag.test();
  }

  void interrupt() { m_interrupt_flag.test_and_set(); }

  void kill_check() override {
    assert(!m_current_check.name.empty());  // be sure we have a check reported
    try {
      if (!m_kill_session) {
        m_kill_session = clone_session(m_main_session);
      }
      if (!m_current_check.kill_session_required) {
        m_kill_session->execute("KILL QUERY " +
                                std::to_string(m_current_check.connection_id));
      } else {
        m_kill_session->execute("KILL CONNECTION " +
                                std::to_string(m_current_check.connection_id));
      }
    } catch (const std::exception &ex) {
      current_console()->print_error(
          shcore::str_format("Error terminating check %.*s: %s",
                             static_cast<int>(m_current_check.name.length()),
                             m_current_check.name.data(), ex.what()));
      throw;
    }
  }

 private:
  void on_exception(std::exception_ptr ex) override { m_thread_exception = ex; }

  const Check_info &current_check() const override { return m_current_check; }

  bool wait() override {
    assert(m_wait_time.has_value());
    return m_checklist_condition.wait_for(m_execution_lock, *m_wait_time,
                                          [this] { return !(is_executing()); });
  }

  bool is_interrupted() const override { return m_interrupt_flag.test(); }

  std::mutex m_checklist_mutex;
  std::unique_lock<std::mutex> m_execution_lock;
  std::condition_variable m_checklist_condition;
  Check_info m_current_check;
  shcore::atomic_flag m_execution_active;
  std::exception_ptr m_thread_exception;
  std::atomic_bool m_check_on_time = true;
  std::optional<std::chrono::seconds> m_wait_time;
  shcore::atomic_flag m_interrupt_flag;
  std::shared_ptr<mysqlshdk::db::ISession> m_main_session;
  std::shared_ptr<mysqlshdk::db::ISession> m_kill_session;
};

struct Session_context {
  Session_context(std::shared_ptr<mysqlshdk::db::ISession> session,
                  bool do_clone_session)
      : m_session(do_clone_session ? clone_session(session) : session),
        m_is_cloned_session(do_clone_session) {}

  Session_context(const Session_context &) = delete;

  Session_context(Session_context &&) = default;

  ~Session_context() {
    if (m_is_cloned_session) {
      m_session->close();
    }
  }

  std::shared_ptr<mysqlshdk::db::ISession> &session() { return m_session; }

 private:
  std::shared_ptr<mysqlshdk::db::ISession> m_session;
  const bool m_is_cloned_session;
};

}  // namespace

using mysqlshdk::utils::Version;
namespace suggested_version {
/*
 * RULE: Do NOT skip bugfix or LTS versions.
 *
 * bugfix:     5.7.whatever    want-to: 9.0   suggestion=8.0
 * bugfix:     8.0.whatever    want-to: 9.0   suggestion=8.4
 * Innovation: 8.1..8.3        want-to: 9.0   suggestion=8.4
 * LTS:        8.4.whatever    want-to: 10.0  suggestion=9.7
 * Innovation: 9.0..9.6        want-to: 10.0	suggestion=9.7
 * LTS:        9.7.whatever    want-to: 10.7	suggestion=10.7
 * Innovation: 10.0..10.6
 * LTS:        10.7.whatever
 */
std::optional<Version> get_next_suggested_lts_version(
    const Version &version, std::string *out_key = nullptr) {
  std::optional<Version> next_lts;
  decltype(k_latest_versions)::const_iterator item = k_latest_versions.end();

  // versions before 8.0.0 must be first updated to 8.0 series
  if (version < Version(8, 0, 0)) {
    item = k_latest_versions.find("8.0");
  } else if (mysqlshdk::utils::is_lts_release(version) &&
             !(version.get_major() == 8 && version.get_minor() == 0)) {
    // We might be already on the LTS release of the series, so it jumps to
    // the LTS in the next series (If they exist)
    auto first_lts = get_first_lts_version(Version(version.get_major() + 1, 0));
    item = k_latest_versions.find(first_lts.get_short());
  } else {
    // in other cases lets find the latest LTS version in the series
    item = k_latest_versions.find(get_first_lts_version(version).get_short());
  }

  if (item != k_latest_versions.end()) {
    if (out_key) {
      *out_key = item->first;
      return item->second;
    }
  }

  return {};
}

std::string format_suggested_version_message(
    const Version &serverVersion, const Version &targetVersion,
    const std::string &suggestedTargetVersion) {
  return shcore::str_format(
      "Upgrading MySQL Server from version %s to %s is not supported. Please "
      "consider running the check using the following option: targetVersion=%s",
      serverVersion.get_base().c_str(), targetVersion.get_base().c_str(),
      suggestedTargetVersion.c_str());
}
}  // namespace suggested_version

std::string check_for_version_suggestion(const Version &serverVersion,
                                         const Version &targetVersion) {
  std::string suggested_target_version;
  auto suggested = suggested_version::get_next_suggested_lts_version(
      serverVersion, &suggested_target_version);
  if (suggested.value_or(targetVersion) >= targetVersion) return {};

  return suggested_version::format_suggested_version_message(
      serverVersion, targetVersion, suggested_target_version);
}

void run_monitoring_thread(IMonitoring_context *context) {
  try {
    Mysql_thread mysql_thread;

    log_debug("Starting monitoring thread.");

    std::string last_check_name;
    while (context->is_executing()) {
      // wait for new data
      if (!context->wait()) {
        if (!context->is_interrupted()) {
          log_debug("Check: %s has timed out.",
                    std::string(context->current_check().name).c_str());

          context->set_check_timedout();
          context->kill_check();
        }
        continue;
      }

      if (context->current_check().name != last_check_name) {
        last_check_name = context->current_check().name;
        log_debug(
            "Execution thread reported new check: %.*s, connection id: "
            "%" PRIu64,
            static_cast<int>(context->current_check().name.length()),
            context->current_check().name.data(),
            context->current_check().connection_id);
      }
    }
  } catch (std::exception &e) {
    log_error(
        "Encountered an exception in Upgrade Checker monitoring thread: %s",
        e.what());
    context->on_exception(std::current_exception());
  }
}

void run_checks(const Upgrade_check_config &config,
                Upgrade_check_output_formatter *print,
                const std::vector<std::unique_ptr<Upgrade_check>> *checklist,
                Stats_info *stats, Execution_context *context) {
  log_debug("Starting upgrade checker execution.");
  auto on_leave = shcore::on_leave_scope([context]() {
    log_debug("Ending upgrade checker execution.");
    context->stop();
    context->wake_up();
  });

  Checker_cache cache{config.db_filters()};

  for (const auto &check : *checklist) {
    if (!context->is_executing()) {
      break;
    }

    auto check_session =
        Session_context(config.session(), check->is_custom_session_required());

    context->on_new_check({check->get_name(),
                           check_session.session()->get_connection_id(),
                           check->is_custom_session_required()});

    // running check
    if (check->is_runnable()) {
      try {
        print->check_title(*check);

        const auto issues = config.filter_issues(
            check->run(check_session.session(), config.upgrade_info(), &cache));
        for (const auto &issue : issues) {
          stats->update(issue.level);
        }
        print->check_results(*check, issues);
      } catch (const Check_configuration_error &e) {
        print->check_error(*check, e.what(), false);
      } catch (const mysqlshdk::db::Error &e) {
        if (context && context->is_check_timedout(check, e)) {
          print->check_error(
              *check, "Warning: Check timed out and was interrupted.", false);
          stats->warnings++;
        } else {
          print->check_error(*check, e.what());
        }
      } catch (const std::exception &e) {
        print->check_error(*check, e.what());
      }
    } else {
      stats->update(dynamic_cast<Manual_check *>(check.get())->get_level());
      print->manual_check(*check);
    }
  }
}

bool run_checks_for_upgrade(const Upgrade_check_config &config,
                            Upgrade_check_output_formatter &print) {
  assert(config.session());

  print.check_info(
      config.session()->get_connection_options().uri_endpoint(),
      config.upgrade_info().server_version_long,
      config.upgrade_info().target_version.get_base(),
      config.upgrade_info().explicit_target_version,
      check_for_version_suggestion(config.upgrade_info().server_version,
                                   config.upgrade_info().target_version));
  config.upgrade_info().validate();

  Upgrade_check_registry::Upgrade_check_vec rejected;
  const auto checklist = Upgrade_check_registry::create_checklist(
      config, false, config.warn_on_excludes() ? &rejected : nullptr);

  Stats_info stats;
  {
    Execution_context context(config.session(), config.has_check_timeout()
                                                    ? config.check_timeout()
                                                    : 0);

    shcore::Interrupt_handler interrupt(
        [&]() {
          context.interrupt();
          context.stop();
          context.kill_check();
          return true;
        },
        [&]() { context.wake_up(); });

    std::thread monitoring_thread;
    if (config.has_check_timeout()) {
      // running monitoring thread
      monitoring_thread =
          mysqlsh::detail::spawn_scoped_thread(run_monitoring_thread, &context);
    }

    auto on_leave = shcore::on_leave_scope([&]() {
      log_debug("Ending monitoring thread.");
      context.stop();
      if (config.has_check_timeout()) {
        monitoring_thread.join();
      }
    });

    run_checks(config, &print, &checklist, &stats, &context);

    if (auto exception = context.exception()) {
      std::rethrow_exception(exception);
    }
  }

  std::string summary;
  if (stats.errors > 0) {
    summary = shcore::str_format(
        "%i errors were found. Please correct these issues before upgrading "
        "to avoid compatibility issues.",
        stats.errors);
  } else if ((stats.warnings > 0) || (stats.notices > 0)) {
    summary =
        "No fatal errors were found that would prevent an upgrade, "
        "but some potential issues were detected. Please ensure that the "
        "reported issues are not significant before upgrading.";
  } else {
    summary = "No known compatibility errors or issues were found.";
  }

  std::map<std::string, std::string> excluded;
  if (config.warn_on_excludes() && !config.exclude_list().empty()) {
    for (const auto &check : rejected) {
      if (!config.exclude_list().contains(check->get_name())) continue;

      excluded[check->get_name()] = check->get_title();
    }
  }

  print.summarize(stats.errors, stats.warnings, stats.notices, summary,
                  excluded);

  return 0 == stats.errors;
}

bool list_checks_for_upgrade(const Upgrade_check_config &config,
                             Upgrade_check_output_formatter &print) {
  if (config.session()) {
    print.list_info(config.session()->get_connection_options().uri_endpoint(),
                    config.upgrade_info().server_version_long,
                    config.upgrade_info().target_version.get_base(),
                    config.upgrade_info().explicit_target_version);
  } else {
    print.list_info();
  }

  config.upgrade_info().validate(true);

  Upgrade_check_registry::Upgrade_check_vec rejected;

  const auto accepted = Upgrade_check_registry::create_checklist(
      config, config.session() == nullptr, &rejected);

  print.list_item_infos("Included", accepted);
  print.list_item_infos("Excluded", rejected);

  print.list_summarize(accepted.size(), rejected.size());

  return true;
}

/*
 * Upgrade Checker entry point
 */
bool check_for_upgrade(const Upgrade_check_config &config) {
  if (!config.list_checks()) {
    if (config.user_privileges()) {
      if (config.user_privileges()
              ->validate({"PROCESS", "SELECT"})
              .has_missing_privileges()) {
        throw std::runtime_error(
            "The upgrade check needs to be performed by user with PROCESS, and "
            "SELECT privileges.");
      }
    } else {
      log_warning(
          "User privileges were not validated, upgrade check may fail.");
    }
  }

  const auto print = config.formatter();
  assert(print);

  if (config.list_checks()) {
    return list_checks_for_upgrade(config, *print);
  }
  return run_checks_for_upgrade(config, *print);
}

}  // namespace upgrade_checker
}  // namespace mysqlsh
