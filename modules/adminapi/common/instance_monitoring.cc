/*
 * Copyright (c) 2019, 2020, Oracle and/or its affiliates. All rights reserved.
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

#include "modules/adminapi/common/instance_monitoring.h"
#include "modules/adminapi/common/common.h"
#include "modules/adminapi/common/dba_errors.h"
#include "mysqlshdk/include/shellcore/interrupt_handler.h"
#include "mysqlshdk/libs/db/mysql/session.h"
#include "mysqlshdk/libs/textui/progress.h"
#include "mysqlshdk/libs/textui/textui.h"
#include "mysqlshdk/libs/utils/utils_general.h"

namespace mysqlsh {
namespace dba {

std::shared_ptr<mysqlsh::dba::Instance> wait_server_startup(
    const mysqlshdk::db::Connection_options &instance_def, int timeout,
    Recovery_progress_style progress_style) {
  mysqlshdk::textui::Spinny_stick stick("* Waiting for server restart...");

  std::shared_ptr<mysqlsh::dba::Instance> out_instance;

  if (progress_style == Recovery_progress_style::NOINFO) {
    stick.done("");
  }

  timeout = adjust_timeout(timeout, k_server_restart_poll_interval_ms);
  while (timeout > 0) {
    try {
      out_instance = Instance::connect(instance_def);

      if (progress_style != Recovery_progress_style::NOWAIT &&
          progress_style != Recovery_progress_style::NOINFO) {
        stick.done("ready");
      }

      log_info("%s has started", out_instance->get_canonical_address().c_str());
      return out_instance;
    } catch (const shcore::Error &e) {
      log_debug2("While waiting for server to start: %s", e.format().c_str());

      if (e.code() == ER_SERVER_SHUTDOWN ||
          mysqlshdk::db::is_mysql_client_error(e.code())) {
        // still not started
      } else {
        if (progress_style != Recovery_progress_style::NOWAIT &&
            progress_style != Recovery_progress_style::NOINFO) {
          stick.done("error");
        }
        throw;
      }
    }
    if (progress_style != Recovery_progress_style::NOWAIT &&
        progress_style != Recovery_progress_style::NOINFO) {
      stick.update();
    }
    timeout--;
    shcore::sleep_ms(k_server_restart_poll_interval_ms);
  }

  if (progress_style != Recovery_progress_style::NOWAIT &&
      progress_style != Recovery_progress_style::NOINFO) {
    stick.done("timeout");
  }

  throw shcore::Exception("Timeout waiting for server to restart",
                          SHERR_DBA_SERVER_RESTART_TIMEOUT);
}

}  // namespace dba
}  // namespace mysqlsh
