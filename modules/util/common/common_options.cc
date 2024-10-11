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

#include "modules/util/common/common_options.h"

#include <utility>

#include "mysqlshdk/include/scripting/type_info/custom.h"
#include "mysqlshdk/include/scripting/type_info/generic.h"
#include "mysqlshdk/include/shellcore/console.h"
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/mysql/instance.h"
#include "mysqlshdk/libs/oci/oci_par.h"
#include "mysqlshdk/libs/utils/logger.h"
#include "mysqlshdk/libs/utils/utils_general.h"
#include "mysqlshdk/libs/utils/utils_string.h"

#include "modules/mod_utils.h"
#include "modules/util/dump/errors.h"
#include "modules/util/load/load_errors.h"

namespace mysqlsh {
namespace common {

const shcore::Option_pack_def<Common_options> &Common_options::options() {
  static const auto opts =
      shcore::Option_pack_def<Common_options>()
          .include<Storage_options>()
          .optional("showProgress", &Common_options::set_show_progress)
          .on_log(&Common_options::on_log_options);

  return opts;
}

Common_options::Common_options(Config config)
    : Storage_options(config.reads_files),
      m_config(std::move(config)),
      m_show_progress(isatty(STDIN_FILENO)) {}

void Common_options::on_log_options(const std::string &msg) const {
  log_info("%s() options: %s", m_config.name,
           mysqlshdk::oci::mask_any_par(msg).c_str());
}

void Common_options::set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  on_set_session(session);

  m_session = session;
  m_connection_options = get_classic_connection_options(m_session);

  {
    // Set long timeouts by default
    constexpr auto timeout = 86400000;  // 1 day in milliseconds

    if (!m_connection_options.has_net_read_timeout()) {
      m_connection_options.set_net_read_timeout(timeout);
    }

    if (!m_connection_options.has_net_write_timeout()) {
      m_connection_options.set_net_write_timeout(timeout);
    }
  }

  // set size of max packet (~size of 1 row) we can send to server
  if (!m_connection_options.has(mysqlshdk::db::kMaxAllowedPacket)) {
    constexpr auto k_one_gb = "1073741824";
    m_connection_options.set(mysqlshdk::db::kMaxAllowedPacket, k_one_gb);
  }

  if (m_config.uses_local_infile) {
    if (m_connection_options.has(mysqlshdk::db::kLocalInfile)) {
      m_connection_options.remove(mysqlshdk::db::kLocalInfile);
    }

    m_connection_options.set(mysqlshdk::db::kLocalInfile, "true");
  }
}

void Common_options::on_set_session(
    const std::shared_ptr<mysqlshdk::db::ISession> &session) {
  if (m_config.uses_local_infile) {
    const auto result =
        session->query("SHOW GLOBAL VARIABLES LIKE 'local_infile'");
    const auto row = result->fetch_one_or_throw();
    const auto local_infile_value = row->get_string(1);

    if (shcore::str_caseeq(local_infile_value, "off")) {
      mysqlsh::current_console()->print_error(
          "The 'local_infile' global system variable must be set to ON in the "
          "target server, after the server is verified to be trusted.");
      THROW_ERROR(SHERR_LOAD_LOCAL_INFILE_DISABLED);
    }
  }
}

std::string Common_options::canonical_address() const {
  return mysqlshdk::mysql::Instance{session()}.get_canonical_address();
}

void Common_options::validate_and_configure() {
  on_validate();
  on_configure();
}

}  // namespace common
}  // namespace mysqlsh
