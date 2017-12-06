/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYSQLSHDK_LIBS_DB_REPLAY_SETUP_H_
#define MYSQLSHDK_LIBS_DB_REPLAY_SETUP_H_

#include <map>
#include <memory>
#include <string>

#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/db/mysqlx/session.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/db/replay/recorder.h"
#include "mysqlshdk/libs/db/replay/replayer.h"

namespace mysqlshdk {
namespace db {
namespace replay {

enum class Mode { Direct, Record, Replay };

void setup_from_env();
void set_mode(Mode mode);

void set_recording_path_prefix(const std::string& path);
void set_recording_context(const std::string& context);

void setup_mysql_session_injector(Mode mode);

void set_replay_query_hook(Query_hook func);
void set_replay_row_hook(Result_row_hook func);

//
// void setup_mysqlx_session_injector(Mode mode) {
//   switch (mode) {
//     case Mode::Direct:
//       mysqlx::Session::set_factory_function({});
//       break;
//     case Mode::Record:
//       mysqlx::Session::set_factory_function(replay::Recorder::create_mysqlx);
//       break;
//     case Mode::Replay:
//       mysqlx::Session::set_factory_function(replay::Replayer::create_mysqlx);
//       break;
//   }
// }


std::string current_recording_dir();

std::string mysqlprovision_recording_path();
std::string new_recording_path(const std::string &type);
std::string next_replay_path(const std::string &type);

void save_test_case_info(const std::map<std::string, std::string> &state);
std::map<std::string, std::string> load_test_case_info();

class No_replay {
 public:
  No_replay();
  ~No_replay();
 private:
  Mode _old_mode;
};

extern char g_recording_path_prefix[1024];
extern int g_session_create_index;
extern int g_session_replay_index;
extern int g_mysqlprovision_prefix_index;
extern Mode g_replay_mode;

}  // namespace replay
}  // namespace db
}  // namespace mysqlshdk

#endif  // MYSQLSHDK_LIBS_DB_REPLAY_SETUP_H_
