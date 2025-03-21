/*
 * Copyright (c) 2024, 2025, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_COMMON_COMMON_OPTIONS_H_
#define MODULES_UTIL_COMMON_COMMON_OPTIONS_H_

#include <memory>
#include <string>

#include "mysqlshdk/include/scripting/types/option_pack.h"

#include "mysqlshdk/libs/db/connection_options.h"
#include "mysqlshdk/libs/db/session.h"

#include "modules/util/common/storage_options.h"

namespace mysqlsh {
namespace common {

class Common_options : public Storage_options {
 public:
  struct Config {
    const char *name;
    const char *action;
    bool reads_files;
    bool uses_local_infile;
    bool url_is_directory;
  };

  Common_options(const Common_options &other) = default;
  Common_options(Common_options &&other) = default;

  Common_options &operator=(const Common_options &other) = default;
  Common_options &operator=(Common_options &&other) = default;

  ~Common_options() override = default;

  static const shcore::Option_pack_def<Common_options> &options();

  void set_url(const std::string &url);

  void set_session(const std::shared_ptr<mysqlshdk::db::ISession> &session);

  const mysqlshdk::db::Connection_options &connection_options() const noexcept {
    return m_connection_options;
  }

  std::string canonical_address() const;

  bool show_progress() const noexcept { return m_show_progress; }

  void validate_and_configure();

 protected:
  explicit Common_options(Config config);

  virtual void on_set_url(const std::string &url, Storage_type storage,
                          const mysqlshdk::storage::Config_ptr &config);

  virtual void on_set_session(
      const std::shared_ptr<mysqlshdk::db::ISession> &session);

  virtual void on_validate() const {}

  virtual void on_configure() {}

  const std::string &url() const noexcept { return m_url; }

  const std::shared_ptr<mysqlshdk::db::ISession> &session() const noexcept {
    return m_session;
  }

  void set_show_progress(bool show) { m_show_progress = show; }

  void throw_on_url_and_storage_conflict(const std::string &url) const;

 private:
  void on_log_options(const std::string &msg) const;

  Config m_config;

  std::string m_url;

  std::shared_ptr<mysqlshdk::db::ISession> m_session;
  mysqlshdk::db::Connection_options m_connection_options;

  bool m_show_progress;
};

}  // namespace common
}  // namespace mysqlsh

#endif  // MODULES_UTIL_COMMON_COMMON_OPTIONS_H_
