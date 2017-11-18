/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "mysqlshdk/libs/db/replay/setup.h"
#include "mysqlshdk/libs/db/replay/trace.h"
#include "mysqlshdk/libs/utils/utils_file.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

namespace mysqlshdk {
namespace db {
namespace replay {

char g_recording_path_prefix[1024] = {0};
char g_recording_context[1024] = {0};
int g_session_create_index = 0;
int g_session_replay_index = 0;
int g_mysqlprovision_prefix_index = 0;
Mode g_replay_mode = Mode::Direct;
Result_row_hook g_replay_row_hook;
Query_hook g_replay_query_hook;

void set_recording_path_prefix(const std::string& path) {
  assert(path.size() + 1 < sizeof(g_recording_path_prefix));
  if (!path.empty()) {
    snprintf(g_recording_path_prefix, sizeof(g_recording_path_prefix), "%s",
             path.c_str());
  } else {
    g_recording_path_prefix[0] = 0;
  }
  g_session_create_index = 0;
  g_session_replay_index = 0;
  g_mysqlprovision_prefix_index = 0;
}

void set_recording_context(const std::string& context) {
  assert(context.size() < sizeof(g_recording_context));
  snprintf(g_recording_context, sizeof(g_recording_context), "%s",
           context.c_str());
  g_session_create_index = 0;
  g_session_replay_index = 0;
  g_mysqlprovision_prefix_index = 0;
}

void set_mode(Mode mode) {
  g_replay_mode = mode;
  setup_mysql_session_injector(mode);
}

std::string current_recording_dir() {
  std::string path = g_recording_path_prefix;
  path.append(g_recording_context);
  return path;
}

std::string mysqlprovision_recording_path() {
  ++g_mysqlprovision_prefix_index;
  std::string path = g_recording_path_prefix;

  path.append(g_recording_context);
  if (!shcore::str_endswith(g_recording_context, "/"))
    path.append("_");
  path.append("mp");
  path.append(std::to_string(g_mysqlprovision_prefix_index));
  return path;
}

std::string new_recording_path(const std::string &type) {
  g_session_create_index++;
  std::string path = g_recording_path_prefix;
  char* p = strrchr(g_recording_context, '/');
  if (p) {
    shcore::ensure_dir_exists(path + "/" + std::string(g_recording_context, p));
  }
  path.append(g_recording_context);
  if (!shcore::str_endswith(g_recording_context, "/"))
    path.append(".");
  path.append(std::to_string(g_session_create_index));
  path.append("." + type);
  return path;
}

std::string next_replay_path(const std::string &type) {
  g_session_replay_index++;
  std::string path = g_recording_path_prefix;
  path.append(g_recording_context);
  if (!shcore::str_endswith(g_recording_context, "/"))
    path.append(".");
  path.append(std::to_string(g_session_replay_index));
  path.append("." + type);
  return path;
}

void save_test_case_info(const std::map<std::string, std::string>& state) {
  save_info(current_recording_dir() + "/info", state);
}

std::map<std::string, std::string> load_test_case_info() {
  return load_info(current_recording_dir() + "/info");
}

void setup_from_env() {
  if (const char* mode = getenv("MYSQLSH_RECORDER_MODE")) {
    if (strcasecmp(mode, "direct") == 0 || !*mode) {
      set_mode(Mode::Direct);
      puts("Disabled classic session recording");
    } else if (strcasecmp(mode, "record") == 0) {
      set_mode(Mode::Record);

      if (!getenv("MYSQLSH_RECORDER_PREFIX")) {
        printf(
            "MYSQLSH_RECORDER_MODE set but MYSQLSH_RECORDER_PREFIX is not!\n");
        return;
      }
      set_recording_path_prefix(getenv("MYSQLSH_RECORDER_PREFIX"));

      printf("Recording classic sessions to %s\n", g_recording_path_prefix);
    } else if (strcasecmp(mode, "replay") == 0) {
      set_mode(Mode::Replay);

      if (!getenv("MYSQLSH_RECORDER_PREFIX")) {
        printf(
            "MYSQLSH_RECORDER_MODE set but MYSQLSH_RECORDER_PREFIX is not!\n");
        return;
      }
      set_recording_path_prefix(getenv("MYSQLSH_RECORDER_PREFIX"));
      printf("Replaying classic sessions from %s\n", g_recording_path_prefix);
    } else {
      printf("Invalid value for MYSQLSH_RECORDER_MODE '%s'\n", mode);
    }
    if (getenv("PAUSE_ON_START"))
      shcore::sleep_ms(10000);
  }
}


static Mode g_active_session_injector_mode = Mode::Direct;

void setup_mysql_session_injector(Mode mode) {
  g_active_session_injector_mode = mode;
  switch (mode) {
    case Mode::Direct:
      mysql::Session::set_factory_function({});
      mysqlx::Session::set_factory_function({});
      break;
    case Mode::Record:
      mysql::Session::set_factory_function(replay::Recorder_mysql::create);
      mysqlx::Session::set_factory_function(replay::Recorder_mysqlx::create);
      break;
    case Mode::Replay:
      mysql::Session::set_factory_function(replay::Replayer_mysql::create);
      mysqlx::Session::set_factory_function(replay::Replayer_mysqlx::create);
      break;
  }
}

void set_replay_row_hook(Result_row_hook func) {
  g_replay_row_hook = func;
}

void set_replay_query_hook(Query_hook func) {
  g_replay_query_hook = func;
}

No_replay::No_replay() {
  _old_mode = g_active_session_injector_mode;
  if (_old_mode != Mode::Direct)
    setup_mysql_session_injector(Mode::Direct);
}

No_replay::~No_replay() {
  if (g_active_session_injector_mode != _old_mode)
    setup_mysql_session_injector(_old_mode);
}

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk
