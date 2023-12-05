/*
 * Copyright (c) 2017, 2023, Oracle and/or its affiliates.
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

#ifndef MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_H_
#define MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_H_

#include <memory>
#include <vector>

#include "modules/util/upgrade_checker/common.h"
#include "mysqlshdk/libs/db/session.h"
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlsh {
namespace upgrade_checker {

class Upgrade_check {
 public:
  explicit Upgrade_check(const char *name) : m_name(name) {}
  virtual ~Upgrade_check() {}

  virtual const char *get_name() const { return m_name; }
  virtual const char *get_title() const;
  virtual const char *get_description() const;
  virtual const char *get_doc_link() const;
  virtual Upgrade_issue::Level get_level() const = 0;
  virtual bool is_runnable() const { return true; }
  virtual bool is_multi_lvl_check() const { return false; }

  virtual std::vector<Upgrade_issue> run(
      const std::shared_ptr<mysqlshdk::db::ISession> &session,
      const Upgrade_info &server_info) = 0;

  virtual const char *get_description_internal() const { return nullptr; }
  virtual const char *get_doc_link_internal() const { return nullptr; }
  virtual const char *get_title_internal() const { return nullptr; }

  const char *get_text(const char *field) const;

 private:
  const char *m_name;
};

}  // namespace upgrade_checker
}  // namespace mysqlsh

#endif  // MODULES_UTIL_UPGRADE_CHECKER_UPGRADE_CHECK_H_