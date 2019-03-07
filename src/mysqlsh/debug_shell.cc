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

#include "dbug/my_dbug.h"
#include "modules/util/upgrade_check.h"
#include "mysqlsh/cmdline_shell.h"
#include "mysqlshdk/libs/db/replay/setup.h"
#include "unittest/test_utils/mod_testutils.h"

using mysqlshdk::db::replay::Mode;
extern const char *g_mysqlsh_path;

// Needed by testutil
bool g_test_color_output = false;
int g_test_trace_scripts = 0;

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
#ifndef _MSC_VER
      std::string filename("/tmp/");
#else
      char buf[MAX_PATH];
      GetTempPathA(MAX_PATH, buf);
      std::string filename(buf);
#endif
      filename += "upgrade_checker.msg";
      if (strlen((*argv)[i]) > strlen("--generate-uc-translation") + 1)
        filename = (*argv)[i] + strlen("--generate-uc-translation") + 1;
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
}

void finalize_debug_shell(mysqlsh::Command_line_shell *shell) {
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
}
