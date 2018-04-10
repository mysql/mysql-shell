/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "mysqlshdk/libs/db/replay/setup.h"
#include <list>
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
int g_external_program_index = 0;
Mode g_replay_mode = Mode::Direct;
Result_row_hook g_replay_row_hook;
Query_hook g_replay_query_hook;

void set_recording_path_prefix(const std::string &path) {
  assert(path.size() + 1 < sizeof(g_recording_path_prefix));
  if (!path.empty()) {
    snprintf(g_recording_path_prefix, sizeof(g_recording_path_prefix), "%s",
             path.c_str());
  } else {
    g_recording_path_prefix[0] = 0;
  }
  g_session_create_index = 0;
  g_session_replay_index = 0;
  g_external_program_index = 0;
}

void begin_recording_context(const std::string &context) {
  assert(context.size() < sizeof(g_recording_context));
  snprintf(g_recording_context, sizeof(g_recording_context), "%s",
           context.c_str());
  g_session_create_index = 0;
  g_session_replay_index = 0;
  g_external_program_index = 0;
}

void end_recording_context() {
  mysqlshdk::db::replay::set_mode(mysqlshdk::db::replay::Mode::Direct, 0);
  mysqlshdk::db::replay::set_replay_row_hook({});
}

void set_mode(Mode mode, int print_traces) {
  g_replay_mode = mode;
  setup_mysql_session_injector(mode, print_traces);
}

std::string current_recording_dir() {
  std::string path = g_recording_path_prefix;
  path.append(g_recording_context);
  return path;
}

std::string external_recording_path(const std::string &program_id) {
  ++g_external_program_index;
  std::string path = g_recording_path_prefix;

  path.append(g_recording_context);
  if (!shcore::str_endswith(g_recording_context, "/")) path.append("_");
  path.append(program_id);
  path.append(std::to_string(g_external_program_index));
  return path;
}

std::string new_recording_path(const std::string &type) {
  g_session_create_index++;
  std::string path = g_recording_path_prefix;
  char *p = strrchr(g_recording_context, '/');
  if (p) {
    shcore::ensure_dir_exists(path + "/" + std::string(g_recording_context, p));
  }
  path.append(g_recording_context);
  if (!shcore::str_endswith(g_recording_context, "/")) path.append(".");
  path.append(std::to_string(g_session_create_index));
  path.append("." + type);
  return path;
}

std::string next_replay_path(const std::string &type) {
  g_session_replay_index++;
  std::string path = g_recording_path_prefix;
  path.append(g_recording_context);
  if (!shcore::str_endswith(g_recording_context, "/")) path.append(".");
  path.append(std::to_string(g_session_replay_index));
  path.append("." + type);
  return path;
}

void save_test_case_info(const std::map<std::string, std::string> &state) {
  save_info(current_recording_dir() + "/info", state);
}

std::map<std::string, std::string> load_test_case_info() {
  return load_info(current_recording_dir() + "/info");
}

static Mode g_active_session_injector_mode = Mode::Direct;
static int g_active_session_print_trace_mode = 0;

void setup_mysql_session_injector(Mode mode, int print_traces) {
  g_active_session_injector_mode = mode;
  g_active_session_print_trace_mode = print_traces;
  switch (mode) {
    case Mode::Direct:
      mysql::Session::set_factory_function({});
      mysqlx::Session::set_factory_function({});
      break;
    case Mode::Record:
      mysql::Session::set_factory_function([print_traces]() {
        return std::shared_ptr<mysql::Session>(
            new replay::Recorder_mysql(print_traces));
      });
      mysqlx::Session::set_factory_function([print_traces]() {
        return std::shared_ptr<mysqlx::Session>(
            new replay::Recorder_mysqlx(print_traces));
      });
      break;
    case Mode::Replay:
      mysql::Session::set_factory_function([print_traces]() {
        return std::shared_ptr<mysql::Session>(
            new replay::Replayer_mysql(print_traces));
      });
      mysqlx::Session::set_factory_function([print_traces]() {
        return std::shared_ptr<mysqlx::Session>(
            new replay::Replayer_mysqlx(print_traces));
      });
      break;
  }
}

void set_replay_row_hook(Result_row_hook func) { g_replay_row_hook = func; }

void set_replay_query_hook(Query_hook func) { g_replay_query_hook = func; }

No_replay::No_replay() {
  _old_mode = g_active_session_injector_mode;
  if (_old_mode != Mode::Direct)
    setup_mysql_session_injector(Mode::Direct,
                                 g_active_session_print_trace_mode);
}

No_replay::~No_replay() {
  if (g_active_session_injector_mode != _old_mode)
    setup_mysql_session_injector(_old_mode, g_active_session_print_trace_mode);
}

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk
