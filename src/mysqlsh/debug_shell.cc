/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
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
#include "mysqlsh/cmdline_shell.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/utils/debug.h"
#include "mysqlshdk/libs/utils/utils_path.h"
#include "mysqlshdk/libs/utils/utils_process.h"
#include "unittest/test_utils/mod_testutils.h"
#include "unittest/test_utils/test_net_utilities.h"

using mysqlshdk::db::replay::Mode;
extern const char *g_mysqlsh_path;

// Needed by testutil
int g_test_color_output = 0;
int g_test_trace_scripts = 0;
bool g_bp = false;

namespace {
std::list<std::weak_ptr<mysqlshdk::db::ISession>> g_open_sessions;

void on_session_connect(std::shared_ptr<mysqlshdk::db::ISession> session) {
  // called by session recorder classes when connect is called
  // adds a weak ptr to the session object along with the stack trace
  // to a list of open sessions, which will be checked when the test finishes
  g_open_sessions.push_back(session);
}

void on_session_close(std::shared_ptr<mysqlshdk::db::ISession> session) {
  // called by session recorder classes when close is called
  for (auto iter = g_open_sessions.begin(); iter != g_open_sessions.end();
       ++iter) {
    auto ptr = iter->lock();
    if (ptr && ptr.get() == session.get()) {
      g_open_sessions.erase(iter);
      break;
    }
  }
}

class Devutil : public mysqlsh::Extensible_object {
 public:
  Devutil(shcore::IShell_core *owner)
      : mysqlsh::Extensible_object("devutil", "devutil", true), m_shell(owner) {
    expose("query", &Devutil::query_mock, "sql");
  }

  void query_mock(const std::string &query) {
    if (!m_shell->get_dev_session()) {
      throw std::runtime_error("Shell is not connected");
    }
    std::shared_ptr<mysqlshdk::db::ISession> session =
        m_shell->get_dev_session()->get_core_session();

    auto result = session->querys(query.c_str(), query.size());

    std::string output = "->expect_query(R\"*(";
    output.append(query);
    output.append(")*\")\n.then_return({{\"\", {");
    bool first = true;
    for (const auto &col : result->get_metadata()) {
      if (!first) output.append(", ");
      first = false;
      output.append("\"").append(col.get_column_label()).append("\"");
    }
    output.append("}, {");
    first = true;
    for (const auto &col : result->get_metadata()) {
      if (!first) output.append(", ");
      first = false;
      output.append("Type::").append(mysqlshdk::db::to_string(col.get_type()));
    }
    output.append("}, {");
    first = true;
    while (auto row = result->fetch_one()) {
      if (first)
        output.append("{");
      else
        output.append(", {");
      first = false;
      for (size_t i = 0; i < row->num_fields(); i++) {
        if (i > 0) output.append(", ");
        if (row->is_null(i)) {
          output.append("\"___NULL___\"");
        } else if (mysqlshdk::db::is_string_type(row->get_type(i))) {
          std::string s = row->get_as_string(i);
          s = shcore::str_replace(s, "\\", "\\\\");
          s = shcore::str_replace(s, "\n", "\\n");
          s = shcore::str_replace(s, "\"", "\\\"");
          output.append("\"").append(s).append("\"");
        } else {
          output.append("\"").append(row->get_as_string(i)).append("\"");
        }
      }
      output.append("}");
    }
    output.append("}}})");

    std::cout << output << "\n";
  }

 private:
  shcore::IShell_core *m_shell;
};

tests::Test_net_utilities test_net_utilities;

}  // namespace

void handle_debug_options(int *argc, char ***argv) {
  if (const char *mode = getenv("MYSQLSH_RECORDER_MODE")) {
    if (strcasecmp(mode, "direct") == 0 || !*mode) {
      mysqlshdk::db::replay::set_mode(Mode::Direct);
      puts("Disabled classic session recording");
    } else if (strcasecmp(mode, "record") == 0) {
      mysqlshdk::db::replay::set_mode(Mode::Record);

      if (!getenv("MYSQLSH_RECORDER_PREFIX")) {
        printf(
            "MYSQLSH_RECORDER_MODE set but MYSQLSH_RECORDER_PREFIX is not!\n");
        return;
      }
      mysqlshdk::db::replay::set_recording_path_prefix(
          getenv("MYSQLSH_RECORDER_PREFIX"));

      // Set up hooks for keeping track of opened sessions
      mysqlshdk::db::replay::on_recorder_connect_hook =
          std::bind(&on_session_connect, std::placeholders::_1);
      mysqlshdk::db::replay::on_recorder_close_hook =
          std::bind(&on_session_close, std::placeholders::_1);

      printf("Recording classic sessions to %s\n",
             mysqlshdk::db::replay::g_recording_path_prefix);

      {
        const auto hostname = getenv("MYSQL_HOSTNAME");

        if (hostname) {
          printf("Capturing network utilities on %s\n", hostname);
          test_net_utilities.inject(hostname, {},
                                    mysqlshdk::db::replay::Mode::Record);

          std::map<std::string, std::string> info;
          info["hostname"] = hostname;
          mysqlshdk::db::replay::save_test_case_info(info);
        }
      }
    } else if (strcasecmp(mode, "replay") == 0) {
      mysqlshdk::db::replay::set_mode(Mode::Replay);

      if (!getenv("MYSQLSH_RECORDER_PREFIX")) {
        printf(
            "MYSQLSH_RECORDER_MODE set but MYSQLSH_RECORDER_PREFIX is not!\n");
        return;
      }
      mysqlshdk::db::replay::set_recording_path_prefix(
          getenv("MYSQLSH_RECORDER_PREFIX"));
      printf("Replaying classic sessions from %s\n",
             mysqlshdk::db::replay::g_recording_path_prefix);

      {
        auto info = mysqlshdk::db::replay::load_test_case_info();

        if (info["net_data"] != "") {
          test_net_utilities.inject(info["hostname"],
                                    shcore::Value::parse(info["net_data"]),
                                    mysqlshdk::db::replay::Mode::Replay);
        }
      }
    } else {
      printf("Invalid value for MYSQLSH_RECORDER_MODE '%s'\n", mode);
    }
  }

  for (int j = 0, i = 0, c = *argc; i < c; i++) {
    if (strcmp((*argv)[i], "--trace") == 0) {
      // stop executing script on failure
      g_test_trace_scripts = 2;
      (*argc)--;
    } else if (strcmp((*argv)[i], "--trace-sql") == 0) {
      DBUG_SET("+d,sql");
      (*argc)--;
    } else if (strcmp((*argv)[i], "--trace-all-sql") == 0) {
      DBUG_SET("+d,sql,sqlall");
      (*argc)--;
    } else if (strncmp((*argv)[i], "--record=", strlen("--record=")) == 0) {
      mysqlshdk::db::replay::set_mode(Mode::Record);
      mysqlshdk::db::replay::set_recording_path_prefix(strchr((*argv)[i], '=') +
                                                       1);
      (*argc)--;
    } else if (strncmp((*argv)[i], "--replay=", strlen("--replay=")) == 0) {
      mysqlshdk::db::replay::set_mode(Mode::Replay);
      mysqlshdk::db::replay::set_recording_path_prefix(strchr((*argv)[i], '=') +
                                                       1);
      (*argc)--;
    } else if (strncmp((*argv)[i], "--direct", strlen("--direct")) == 0) {
      mysqlshdk::db::replay::set_mode(Mode::Direct);
      (*argc)--;
    } else if (strncmp((*argv)[i], "--generate-uc-translation",
                       strlen("--generate-uc-translation")) == 0) {
      std::string filename = shcore::path::join_path(shcore::path::tmpdir(),
                                                     "upgrade_checker.msg");
      if (strlen((*argv)[i]) > strlen("--generate-uc-translation") + 1) {
        filename = (*argv)[i] + strlen("--generate-uc-translation") + 1;
      }
      try {
        mysqlsh::Upgrade_check::prepare_translation_file(filename.c_str());
        std::cout << "Upgrade checker translation file written to: " << filename
                  << std::endl;
      } catch (const std::exception &e) {
        std::cerr << "Failed to write upgrade checker translation file: "
                  << e.what() << std::endl;
      }
      exit(0);
    } else {
      (*argv)[j++] = (*argv)[i];
    }
  }
}

void init_debug_shell(std::shared_ptr<mysqlsh::Command_line_shell> shell) {
  tests::Testutils::validate_boilerplate(shell->options().sandbox_directory,
                                         false);

  std::shared_ptr<tests::Testutils> testutil(
      new tests::Testutils(shell->options().sandbox_directory,
                           mysqlshdk::db::replay::g_replay_mode ==
                               mysqlshdk::db::replay::Mode::Replay,
                           shell, g_mysqlsh_path));
  if (mysqlshdk::db::replay::g_replay_mode !=
      mysqlshdk::db::replay::Mode::Direct)
    testutil->set_sandbox_snapshot_dir(
        mysqlshdk::db::replay::current_recording_dir());
  shell->set_global_object("testutil", testutil);

  const auto &opts = shell->options().dbug_options;
  if (!opts.empty()) {
    auto set_one_trap = [testutil](const shcore::Dictionary_t &fispec) {
      std::string type = fispec->get_string("on");
      shcore::Array_t conds =
          fispec->has_key("if") ? fispec->get_array("if") : nullptr;

      if (fispec->has_key("if")) fispec->erase("if");
      fispec->erase("on");

      testutil->set_trap(type, conds, fispec);
    };

    if (opts[0] == '{') {
      // {on:type,if:[conditions],opts}
      set_one_trap(shcore::Value::parse(opts).as_map());
    } else if (opts[0] == '[') {
      // [{on:type,if:[conditions],opts},...]
      shcore::Array_t fispecs = shcore::Value::parse(opts).as_array();
      for (const shcore::Value &v : *fispecs) {
        set_one_trap(v.as_map());
      }
    }
  }

  std::shared_ptr<Devutil> devutil(new Devutil(shell->shell_context().get()));
  shell->set_global_object("devutil", devutil);
}

void finalize_debug_shell(mysqlsh::Command_line_shell *shell) {
  if (Mode::Record == mysqlshdk::db::replay::g_replay_mode) {
    auto info = mysqlshdk::db::replay::load_test_case_info();
    info["net_data"] = test_net_utilities.get_recorded().repr();
    mysqlshdk::db::replay::save_test_case_info(info);
  }

  // Automatically close recording sessions that may be still open
  mysqlshdk::db::replay::on_recorder_connect_hook = {};
  mysqlshdk::db::replay::on_recorder_close_hook = {};

  for (const auto &s : g_open_sessions) {
    if (auto session = s.lock()) {
      session->close();
    }
  }
  g_open_sessions.clear();

  shell->set_global_object("testutil", {});
  shell->set_global_object("devutil", {});

  test_net_utilities.remove();
}
