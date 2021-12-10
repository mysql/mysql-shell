/*
 * Copyright (c) 2021, Oracle and/or its affiliates.
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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#include "mysqlshdk/libs/db/mysql/auth_plugins/mysql_event_handler_plugin.h"

#include <mysql.h>
#include <mysql/plugin_trace.h>
#include <cstring>

#include "mysqlshdk/libs/db/mysql/auth_plugins/fido.h"
#include "mysqlshdk/libs/utils/logger.h"

namespace mysqlshdk {
namespace db {
namespace mysql {
namespace {

struct st_trace_data {
  int last_cmd;
  enum protocol_stage next_stage;
  unsigned int param_count;
  unsigned int col_count;
  bool multi_resultset;
  int errnum;
};

#define protocol_stage_to_str(X) \
  case PROTOCOL_STAGE_##X:       \
    return #X;

const char *protocol_stage_str(enum protocol_stage stage) {
  switch (stage) {
    PROTOCOL_STAGE_LIST(to_str)
    default:
      return "<unknown stage>";
  }
}

#undef protocol_stage_to_str

#define trace_event_to_str(X) \
  case TRACE_EVENT_##X:       \
    return #X;

const char *trace_event_str(enum trace_event ev) {
  switch (ev) {
    TRACE_EVENT_LIST(to_str)
    default:
      return "<unknown event>";
  }
}

#undef trace_event_to_str

#define log_protocol_event(...)                                       \
  do {                                                                \
    auto logger = shcore::current_logger(true);                       \
    if (logger) logger->log(shcore::Logger::LOG_DEBUG3, __VA_ARGS__); \
  } while (false)

void *trace_start(struct st_mysql_client_plugin_TRACE *plugin_data,
                  MYSQL * /*conn*/, enum protocol_stage stage) {
  struct st_trace_data *trace_data = nullptr;

  log_protocol_event("PLUGIN-TRACE-START: %s.%s", plugin_data->name,
                     protocol_stage_str(stage));

  try {
    trace_data = new st_trace_data;
    memset(trace_data, 0, sizeof(st_trace_data));
    trace_data->next_stage = PROTOCOL_STAGE_CONNECTING;
  } catch (const std::bad_alloc &) {
    log_protocol_event(
        "PLUGIN-TRACE-START: Could not allocate per-connection trace data - "
        "checking protocol flow will be disabled");
  }

  return trace_data;
}

void trace_stop(struct st_mysql_client_plugin_TRACE *plugin_data,
                MYSQL * /*conn*/, void *data) {
  log_protocol_event("PLUGIN-TRACE-STOP: %s", plugin_data->name);

  if (data) {
    delete static_cast<st_trace_data *>(data);
  }
}

int trace_event(struct st_mysql_client_plugin_TRACE * /*plugin_data*/,
                void * /*data_ptr*/, MYSQL *conn, enum protocol_stage stage,
                enum trace_event ev, struct st_trace_event_args args) {
  if (args.plugin_name) {
    log_protocol_event("PLUGIN-EVENT: %s.%s.%s", args.plugin_name,
                       protocol_stage_str(stage), trace_event_str(ev));

    if (0 == strcmp(args.plugin_name, "authentication_fido_client")) {
      // Instantiating the fido handler will set the print callback
      fido::register_print_callback(conn);
    }
  }

  return 0;
}

st_mysql_client_plugin_TRACE mysql_plugin_tracer = {
    MYSQL_CLIENT_TRACE_PLUGIN,
    MYSQL_CLIENT_TRACE_PLUGIN_INTERFACE_VERSION,
    "PluginEventHandler",
    "MySQL Shell Team",
    "A plugin to MySQL protocol events",
    {0, 1, 0},
    "GPL",
    NULL,
    NULL,  // init,
    NULL,  // deinit,
    NULL,  // options
    NULL,  // get_options
    trace_start,
    trace_stop,
    trace_event};

}  // namespace

/**
 * Creates a MySQL Plugin to trace server events.
 *
 * Initial use case: detect authentication to be done using a FIDO device to
 * update the authentication_fido plugin a print callback that uses
 * Shell_console
 */

void register_tracer_plugin(MYSQL *conn) {
  mysql_client_register_plugin(
      conn, reinterpret_cast<st_mysql_client_plugin *>(&mysql_plugin_tracer));
}

}  // namespace mysql
}  // namespace db
}  // namespace mysqlshdk
