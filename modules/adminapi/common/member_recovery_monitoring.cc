/*
 * Copyright (c) 2019, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/member_recovery_monitoring.h"
#include "modules/adminapi/common/clone_progress.h"
#include "modules/adminapi/common/dba_errors.h"
#include "modules/adminapi/common/instance_monitoring.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/mysql/replication.h"
#include "mysqlshdk/libs/utils/atomic_flag.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/strformat.h"
#include "mysqlshdk/libs/utils/utils_net.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlsh {
namespace dba {

namespace {

void throw_clone_recovery_error(const mysqlshdk::mysql::IInstance &instance,
                                const std::string &start_time) {
  mysqlshdk::mysql::Clone_status status;

  status = mysqlshdk::mysql::check_clone_status(instance, start_time);

  assert(status.state == mysqlshdk::mysql::k_CLONE_STATE_FAILED);

  auto console = current_console();
  console->print_error("The clone process has failed: " + status.error + " (" +
                       std::to_string(status.error_n) + ")");
  throw shcore::Exception("Clone recovery has failed",
                          SHERR_DBA_CLONE_RECOVERY_FAILED);
}

std::string show_distributed_recovery_error(
    const mysqlshdk::mysql::IInstance &instance,
    const std::string &previous_error_timestamp,
    mysqlshdk::mysql::Replication_channel *out_channel) {
  auto console = current_console();
  mysqlshdk::mysql::Replication_channel channel;

  if (mysqlshdk::mysql::get_channel_status(
          instance, mysqlshdk::gr::k_gr_recovery_channel, &channel)) {
    std::string last_error_time = previous_error_timestamp;

    if (out_channel) *out_channel = channel;

    if (channel.receiver.last_error.code != 0 &&
        last_error_time != channel.receiver.last_error.timestamp) {
      console->print_warning(
          "Error in receiver for " +
          std::string(mysqlshdk::gr::k_gr_recovery_channel) + ": " +
          mysqlshdk::mysql::to_string(channel.receiver.last_error));
      last_error_time = channel.receiver.last_error.timestamp;
    }

    if (!channel.appliers.empty() && channel.appliers[0].last_error.code != 0 &&
        last_error_time != channel.appliers[0].last_error.timestamp) {
      console->print_warning(
          "Error in applier for " +
          std::string(mysqlshdk::gr::k_gr_recovery_channel) + ": " +
          mysqlshdk::mysql::to_string(channel.appliers[0].last_error));
      last_error_time = channel.appliers[0].last_error.timestamp;
    }
    return last_error_time;
  } else {
    log_warning("Replication channel %s was expected to exist, but doesn't",
                mysqlshdk::gr::k_gr_recovery_channel);
  }
  return "";
}

void throw_distributed_recovery_error(
    const mysqlshdk::mysql::IInstance &instance) {
  std::string last_error_time =
      show_distributed_recovery_error(instance, "", nullptr);

  if (!last_error_time.empty()) {
    throw shcore::Exception("Distributed recovery has failed",
                            SHERR_DBA_DISTRIBUTED_RECOVERY_FAILED);
  }
}

std::shared_ptr<mysqlsh::dba::Instance> monitor_clone_recovery(
    mysqlsh::dba::Instance *instance,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int restart_timeout_sec, bool is_group_member) {
  auto console = current_console();
  bool wait_restart = false;
  bool ignore_cancel = false;
  Clone_progress progress(progress_style);

  shcore::atomic_flag stop;
  shcore::Interrupt_handler intr([&stop]() {
    stop.test_and_set();
    return true;
  });

  auto poll_interval_ms = k_clone_status_poll_interval_ms;
  DBUG_EXECUTE_IF("clone_rig_poll_interval", { poll_interval_ms = 10; });

  bool first = true;
  console->print_info("* Waiting for clone to finish...");
  while (!stop.test()) {
    mysqlshdk::mysql::Clone_status status;

    try {
      status = mysqlshdk::mysql::check_clone_status(*instance, begin_time);
      if (first) {
        console->print_note(instance->descr() + " is being cloned from " +
                            status.source);
        first = false;
      }
    } catch (const shcore::Error &e) {
      if (e.code() == ER_SERVER_SHUTDOWN) {
        // From the very same moment the instance starts the shutdown, aborting
        // the process with ctrl-c must be disabled
        ignore_cancel = true;

        progress.on_restart();

        console->print_info();
        console->print_note(instance->descr() + " is shutting down...");
        console->print_info();
      } else if (mysqlshdk::db::is_mysql_client_error(e.code())) {
        progress.on_restart();

        console->print_note(
            "Connection to server lost, restart probably in progress...");
      } else {
        progress.on_error();

        throw;
      }
      wait_restart = true;
      break;
    }

    try {
      progress.update(status);
    } catch (const shcore::Exception &e) {
      std::string err =
          "Clone based recovery has failed: " + std::string(e.what());
      throw shcore::Exception(err, SHERR_DBA_CLONE_RECOVERY_FAILED);
    }

    if (status.state == mysqlshdk::mysql::k_CLONE_STATE_SUCCESS) {
      wait_restart = false;
      break;
    }

    shcore::sleep_ms(poll_interval_ms);
  }
  if (stop.test() && !ignore_cancel) throw stop_monitoring();

  std::shared_ptr<mysqlsh::dba::Instance> new_instance;

  if (wait_restart) {
    ignore_cancel = true;

    // Wait for the server to start again
    try {
      new_instance = wait_server_startup(post_clone_coptions,
                                         restart_timeout_sec, progress_style);
    } catch (const shcore::Exception &e) {
      if (e.code() == SHERR_DBA_SERVER_RESTART_TIMEOUT) {
        throw restart_timeout();
      } else {
        throw;
      }
    }

    console->print_info("* " + instance->get_canonical_address() +
                        " has restarted, waiting for clone to finish...");

    // if start_on_boot is disabled, start GR by hand
    if (is_group_member &&
        !new_instance->get_sysvar_bool("group_replication_start_on_boot")
             .value_or(false)) {
      new_instance->execute("START GROUP_REPLICATION");
    }
  }

  // If a restart happened, use the new pointer. Otherwise, we must use the
  // old pointer to not cause a segfault
  if (!new_instance) {
    new_instance = std::make_shared<mysqlsh::dba::Instance>(*instance);
  }

  // Wait for clone recovery to finish
  while (!stop.test()) {
    mysqlshdk::mysql::Clone_status status;

    status = mysqlshdk::mysql::check_clone_status(*new_instance, begin_time);

    try {
      progress.update(status);
    } catch (const shcore::Exception &e) {
      std::string err =
          "Clone based recovery has failed: " + std::string(e.what());
      throw shcore::Exception(err, SHERR_DBA_CLONE_RECOVERY_FAILED);
    }

    if (status.state == mysqlshdk::mysql::k_CLONE_STATE_SUCCESS) {
      uint64_t total_seconds = status.stages[1].seconds_elapsed +
                               status.stages[2].seconds_elapsed +
                               status.stages[3].seconds_elapsed;
      uint64_t total_bytes = status.stages[1].work_estimated +
                             status.stages[2].work_estimated +
                             status.stages[3].work_estimated;

      std::string stats;
      if (total_seconds < 2) {
        stats = shcore::str_format(
            "%s transferred in about 1 second (~%s)",
            mysqlshdk::utils::format_bytes(total_bytes).c_str(),
            mysqlshdk::utils::format_throughput_bytes(total_bytes,
                                                      total_seconds)
                .c_str());
      } else {
        stats = shcore::str_format(
            "%s transferred in %s (%s)",
            mysqlshdk::utils::format_bytes(total_bytes).c_str(),
            mysqlshdk::utils::format_seconds(total_seconds, false).c_str(),
            mysqlshdk::utils::format_throughput_bytes(total_bytes,
                                                      total_seconds)
                .c_str());
      }
      console->print_info("* Clone process has finished: " + stats);
      console->print_info();
      break;
    }
    shcore::sleep_ms(k_clone_status_poll_interval_ms);
  }
  if (stop.test() && !ignore_cancel) throw stop_monitoring();

  // TODO(miguel/alfredo): refactor this monitoring code to pass the less
  // possible pointers around
  return new_instance;
}

void do_monitor_gr_recovery_status(
    mysqlsh::dba::Instance *instance,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    mysqlshdk::gr::Group_member_recovery_status method,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int startup_timeout_sec, int restart_timeout_sec);

void monitor_post_clone_gr_recovery_status(
    mysqlsh::dba::Instance *instance,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int startup_timeout_sec) {
  shcore::atomic_flag stop;
  shcore::Interrupt_handler intr([&stop]() {
    stop.test_and_set();
    return true;
  });

  auto console = current_console();

  mysqlshdk::gr::Group_member_recovery_status rm =
      mysqlshdk::gr::Group_member_recovery_status::UNKNOWN;

  int startup_timeout_iters =
      adjust_timeout(startup_timeout_sec, k_recovery_status_poll_interval_ms);

  while (startup_timeout_iters > 0 && !stop.test()) {
    try {
      rm = mysqlshdk::gr::detect_recovery_status(*instance, begin_time);
      if (rm != mysqlshdk::gr::Group_member_recovery_status::CLONE) {
        do_monitor_gr_recovery_status(instance, post_clone_coptions, rm,
                                      begin_time, progress_style,
                                      startup_timeout_sec, 0);
        break;
      }
    } catch (const shcore::Error &err) {
      log_warning("During post-clone recovery start check: %s", err.what());
      throw;
    }
    shcore::sleep_ms(k_recovery_status_poll_interval_ms);
    startup_timeout_iters--;
  }

  if (stop.test()) throw stop_monitoring();
}

// show_progress:
// - 0 no wait and no progress
// - 1 wait without progress info
// - 2 wait with textual info only
// - 3 wait with progressbar
void do_monitor_gr_recovery_status(
    mysqlsh::dba::Instance *instance,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    mysqlshdk::gr::Group_member_recovery_status method,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int startup_timeout_sec, int restart_timeout_sec) {
  using mysqlshdk::textui::bold;

  auto console = current_console();

  switch (method) {
    case mysqlshdk::gr::Group_member_recovery_status::CLONE:
    case mysqlshdk::gr::Group_member_recovery_status::CLONE_ERROR:
      console->print_info(
          bold("Clone based state recovery is now in progress."));
      console->print_info();
      console->print_note(mysqlshdk::textui::format_markup_text(
          {"A server restart is expected to happen as part of the clone "
           "process. If the server does not support the RESTART command or "
           "does not come back after a while, you may need to manually start "
           "it back."},
          80));
      console->print_info();
      if (method == mysqlshdk::gr::Group_member_recovery_status::CLONE_ERROR) {
        throw_clone_recovery_error(*instance, begin_time);
        break;
      }

      {
        Scoped_instance new_instance(
            monitor_clone_recovery(instance, post_clone_coptions, begin_time,
                                   progress_style, restart_timeout_sec, true));

        // After a clone, a distributed recovery is done by GR, but there's a
        // time gap between clone finishing and the next recovery starting.
        // We can only be sure that the recovery is over if member_state
        // switches away from RECOVERING, so we need to poll again until we know
        // what's happening.
        monitor_post_clone_gr_recovery_status(
            new_instance.get() ? new_instance.get() : instance,
            post_clone_coptions, begin_time, progress_style,
            startup_timeout_sec);
      }
      break;

    case mysqlshdk::gr::Group_member_recovery_status::DISTRIBUTED:
    case mysqlshdk::gr::Group_member_recovery_status::DISTRIBUTED_ERROR:
      // if clone recovery fails, GR can fallback to distributed recovery
      try {
        mysqlshdk::mysql::Clone_status status =
            mysqlshdk::mysql::check_clone_status(*instance, begin_time);

        if (status.state == mysqlshdk::mysql::k_CLONE_STATE_FAILED) {
          console->print_warning(
              "Clone was attempted, but it has failed with an error: " +
              status.error + " (" + std::to_string(status.error_n) + ")");
          console->print_info("A fallback method will be used.");
        }
      } catch (const shcore::Error &e) {
        if (e.code() != ER_BAD_TABLE_ERROR) {
          log_debug("Error querying clone status: %s", e.format().c_str());
        }
      }
      console->print_info(
          bold("Incremental state recovery is now in progress."));
      console->print_info();
      if (method ==
          mysqlshdk::gr::Group_member_recovery_status::DISTRIBUTED_ERROR) {
        throw_distributed_recovery_error(*instance);
      } else {
        monitor_distributed_recovery(*instance, progress_style);
      }
      break;

    case mysqlshdk::gr::Group_member_recovery_status::UNKNOWN:
    case mysqlshdk::gr::Group_member_recovery_status::UNKNOWN_ERROR:
      console->print_note("Could not detect state recovery method for '" +
                          instance->descr() + "'");
      console->print_info();

      if (method == mysqlshdk::gr::Group_member_recovery_status::UNKNOWN_ERROR)
        console->print_warning(
            "An unknown error occurred in state recovery of the instance.");
      else
        console->print_info(
            "State recovery will continue in background, you may monitor its "
            "state with <Cluster>.status().\n");
      break;

    case mysqlshdk::gr::Group_member_recovery_status::DONE_OFFLINE:
      console->print_info("Group join canceled for '" + instance->descr() +
                          "'");
      console->print_info();
      break;

    case mysqlshdk::gr::Group_member_recovery_status::DONE_ONLINE:
      console->print_info(bold("State recovery already finished for '" +
                               instance->descr() + "'"));
      console->print_info();
      break;
  }
}
}  // namespace

mysqlshdk::gr::Group_member_recovery_status wait_recovery_start(
    const mysqlshdk::db::Connection_options &instance_def,
    const std::string &begin_time, int timeout_sec) {
  // We wait in this loop until:
  // - group_replication_recovery channel pops up
  // - something shows up in PFS.clone_status
  // - state becomes ERROR (recovery failed)
  // - state becomes ONLINE (recovery ended)
  // - state becomes OFFLINE (aborted?)
  // It's also possible that the target instance restarts during our checks.
  // In that case, the instance may or may not come back.

  timeout_sec = adjust_timeout(timeout_sec, k_recovery_status_poll_interval_ms);
  bool reconnect = true;

  Scoped_instance instance;

  shcore::atomic_flag stop;
  shcore::Interrupt_handler intr([&stop]() {
    stop.test_and_set();
    return true;
  });

  while (timeout_sec > 0 && !stop.test()) {
    if (reconnect) {
      try {
        instance = Scoped_instance(wait_server_startup(
            instance_def, timeout_sec, Recovery_progress_style::NONE));
        reconnect = false;
      } catch (const shcore::Exception &e) {
        if (e.code() == SHERR_DBA_SERVER_RESTART_TIMEOUT) break;
        throw;
      }
    }

    if (!reconnect) {
      try {
        mysqlshdk::gr::Group_member_recovery_status rm =
            mysqlshdk::gr::detect_recovery_status(*instance, begin_time);
        if (rm != mysqlshdk::gr::Group_member_recovery_status::UNKNOWN) {
          // We keep trying until we can detect which method is in use
          return rm;
        }
        reconnect = false;
      } catch (const shcore::Error &err) {
        log_warning("During recovery start check: %s", err.what());

        if (mysqlshdk::db::is_mysql_client_error(err.code()) ||
            err.code() == ER_SERVER_SHUTDOWN) {
          // client errors are probably a lost connection, which may mean the
          // instance is restarting
          reconnect = true;
        } else {
          throw;
        }
      }
    }
    shcore::sleep_ms(k_recovery_status_poll_interval_ms);
    timeout_sec--;
  }

  if (stop.test()) throw stop_monitoring();

  return mysqlshdk::gr::Group_member_recovery_status::UNKNOWN;
}

std::shared_ptr<mysqlsh::dba::Instance> wait_clone_start(
    const mysqlshdk::db::Connection_options &instance_def,
    const std::string &begin_time, int timeout_sec) {
  // We wait in this loop until something shows up in PFS.clone_status
  timeout_sec = adjust_timeout(timeout_sec, k_recovery_status_poll_interval_ms);

  std::shared_ptr<mysqlsh::dba::Instance> out_instance;

  bool reconnect = true;

  shcore::atomic_flag stop;
  shcore::Interrupt_handler intr([&stop]() {
    stop.test_and_set();
    return true;
  });

  auto poll_interval_ms = k_recovery_status_poll_interval_ms;
  DBUG_EXECUTE_IF("clone_rig_poll_interval", { poll_interval_ms = 10; });

  while (timeout_sec > 0 && !stop.test()) {
    if (reconnect) {
      try {
        out_instance = wait_server_startup(instance_def, timeout_sec,
                                           Recovery_progress_style::NONE);
        reconnect = false;
      } catch (const shcore::Exception &e) {
        if (e.code() == SHERR_DBA_SERVER_RESTART_TIMEOUT) break;
        throw;
      }
    }

    if (!reconnect) {
      try {
        mysqlshdk::mysql::Clone_status status =
            mysqlshdk::mysql::check_clone_status(*out_instance, begin_time);

        if (!status.state.empty() && !status.stages.empty()) {
          // We keep trying until we can detect clone has started
          break;
        }
        reconnect = false;
      } catch (const shcore::Error &err) {
        log_warning("Error during clone start check: %s", err.format().c_str());

        if (mysqlshdk::db::is_mysql_client_error(err.code()) ||
            err.code() == ER_SERVER_SHUTDOWN) {
          // client errors are probably a lost connection, which may mean the
          // instance is restarting
          reconnect = true;
        } else {
          throw;
        }
      }
    }

    shcore::sleep_ms(poll_interval_ms);
    timeout_sec--;
  }

  if (stop.test()) throw stop_monitoring();

  return out_instance;
}

void monitor_distributed_recovery(const mysqlshdk::mysql::IInstance &instance,
                                  Recovery_progress_style /*progress_style*/) {
  // TODO(.) - show progress
  auto console = mysqlsh::current_console();
  log_debug("Waiting for member_state of %s to become ONLINE...",
            instance.descr().c_str());

  shcore::atomic_flag stop;
  shcore::Interrupt_handler intr([&stop]() {
    stop.test_and_set();
    return true;
  });

  console->print_info("* Waiting for distributed recovery to finish...");
  bool first = true;

  std::string last_error_time;
  while (!stop.test()) {
    mysqlshdk::gr::Member_state state =
        mysqlshdk::gr::get_member_state(instance);

    if (state == mysqlshdk::gr::Member_state::ONLINE) {
      log_debug("State of %s became ONLINE", instance.descr().c_str());
      break;
    } else if (state == mysqlshdk::gr::Member_state::ERROR) {
      log_debug("State of %s became ERROR", instance.descr().c_str());

      throw_distributed_recovery_error(instance);
      break;
    } else if (state == mysqlshdk::gr::Member_state::OFFLINE) {
      // not supposed to happen
      log_debug("State of %s became OFFLINE", instance.descr().c_str());
      break;
    } else {
      mysqlshdk::mysql::Replication_channel channel;

      last_error_time =
          show_distributed_recovery_error(instance, last_error_time, &channel);

      if (first) {
        console->print_note(
            "'" + instance.descr() + "' is being recovered from '" +
            mysqlshdk::utils::make_host_and_port(channel.host, channel.port) +
            "'");
        first = false;
      }
    }
    assert(state == mysqlshdk::gr::Member_state::RECOVERING);

    shcore::sleep_ms(k_recovery_status_poll_interval_ms);
  }

  if (stop.test()) throw stop_monitoring();

  console->print_info("* Distributed recovery has finished");
  console->print_info();
}

void monitor_standalone_clone_instance(
    const mysqlshdk::db::Connection_options &instance_def,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int startup_timeout_sec, int restart_timeout_sec) {
  auto console = current_console();

  console->print_info(
      "Monitoring Clone based state recovery of the new member. Press ^C to "
      "abort the operation.");

  log_info("Waiting for clone process to start at %s...",
           instance_def.uri_endpoint().c_str());

  Scoped_instance instance(
      wait_clone_start(instance_def, begin_time, startup_timeout_sec));

  console->print_info(mysqlshdk::textui::bold(
      "Clone based state recovery is now in progress."));
  console->print_info();
  console->print_note(mysqlshdk::textui::format_markup_text(
      {"A server restart is expected to happen as part of the clone "
       "process. If the server does not support the RESTART command or "
       "does not come back after a while, you may need to manually start "
       "it back."},
      80));
  console->print_info();

  monitor_clone_recovery(instance.get(), post_clone_coptions, begin_time,
                         progress_style, restart_timeout_sec, false);
}

void monitor_gr_recovery_status(
    const mysqlshdk::db::Connection_options &instance_def,
    const mysqlshdk::db::Connection_options &post_clone_coptions,
    const std::string &begin_time, Recovery_progress_style progress_style,
    int startup_timeout_sec, int restart_timeout_sec) {
  auto console = current_console();

  if (progress_style == Recovery_progress_style::MINIMAL) {
    console->print_info(
        "Waiting for recovery process of the new cluster member to complete. "
        "Press ^C to stop waiting and let it continue in background.");
  } else if (progress_style != Recovery_progress_style::MINIMAL) {
    console->print_info(
        "Monitoring recovery process of the new cluster member. Press ^C to "
        "stop monitoring and let it continue in background.");
  }

  // first time, try with target credentials
  auto connection_options = instance_def;

  try {
    log_info("Waiting for GR recovery to start for %s...",
             instance_def.uri_endpoint().c_str());

    DBUG_EXECUTE_IF(
        "sigint_wait_recovery",
        {  // Simulate a SIGINT (ctrl-c) to exit the recovery monitoring
          throw stop_monitoring();
        });

    for (int attempt = 0; attempt < 2; ++attempt) {
      try {
        mysqlshdk::gr::Group_member_recovery_status method =
            wait_recovery_start(connection_options, begin_time,
                                startup_timeout_sec);

        // there's a possibility that mysqlsh::dba::Instance::connect throws an
        // exception while trying to connect, which would then propagate and
        // exit the method without actually being connected. To prevent this,
        // we'll try and call wait_server_startup (just once) and then retry
        // again
        std::shared_ptr<Instance> instance;
        for (int num_retries = 1; num_retries >= 0; --num_retries) {
          try {
            instance = mysqlsh::dba::Instance::connect(instance_def);
            break;
          } catch (const shcore::Error &exp_connect) {
            log_error("Could not open connection to %s: %s",
                      instance_def.uri_endpoint().c_str(), exp_connect.what());

            if ((num_retries > 0) &&
                mysqlshdk::db::is_mysql_client_error(exp_connect.code())) {
              try {
                wait_server_startup(instance_def, startup_timeout_sec,
                                    Recovery_progress_style::NONE);
              } catch (const shcore::Exception &exp_server) {
                if (exp_server.code() != SHERR_DBA_SERVER_RESTART_TIMEOUT)
                  throw;

                log_error("Error waiting for server startup: %d - %s",
                          exp_server.code(), exp_server.what());
              }
              continue;
            }

            // retries exhausted or unknown error, so just rethrow and let the
            // error propagate
            throw;
          }
        }

        do_monitor_gr_recovery_status(instance.get(), post_clone_coptions,
                                      method, begin_time, progress_style,
                                      startup_timeout_sec, restart_timeout_sec);
        break;
      } catch (const shcore::Error &e) {
        if (e.code() == ER_ACCESS_DENIED_ERROR && attempt == 0 &&
            post_clone_coptions.get_user() != instance_def.get_user()) {
          // try again with cluster-wide credentials
          connection_options = post_clone_coptions;
          continue;
        }
        throw;
      }
    }
  } catch (const stop_monitoring &) {
    console->print_info();
    console->print_note(
        "Monitoring of the recovery process was canceled. The recovery itself "
        "will continue executing in background. Use <Cluster>.status() to "
        "check progress.");
  } catch (const shcore::Error &e) {
    throw shcore::Exception::mysql_error_with_code(e.what(), e.code());
  }
}

}  // namespace dba
}  // namespace mysqlsh
