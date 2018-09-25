/*
 * Copyright (c) 2017, 2018 Oracle and/or its affiliates. All rights reserved.
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

#ifndef MODULES_UTIL_UPGRADE_CHECK_H_
#define MODULES_UTIL_UPGRADE_CHECK_H_

#include <forward_list>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "mysqlshdk/libs/utils/version.h"

namespace mysqlshdk {
namespace db {
class ISession;
class IRow;
}  // namespace db
}  // namespace mysqlshdk

namespace mysqlsh {

struct Upgrade_issue {
  enum Level { ERROR = 0, WARNING, NOTICE };
  static const char *level_to_string(const Upgrade_issue::Level level);

  std::string schema;
  std::string table;
  std::string column;
  std::string description;
  Level level = ERROR;

  bool empty() const { return schema.empty(); }
  std::string get_db_object() const;
};

std::string to_string(const Upgrade_issue &problem);

class Upgrade_check {
 public:
  using Creator = std::function<std::unique_ptr<Upgrade_check>(
      const mysqlshdk::utils::Version &, const mysqlshdk::utils::Version &)>;

  using Collection = std::vector<
      std::pair<std::forward_list<mysqlshdk::utils::Version>, Creator>>;

  static const mysqlshdk::utils::Version TRANSLATION_MODE;
  static const mysqlshdk::utils::Version ALL_VERSIONS;

  template <class... Ts>
  static bool register_check(Creator creator, Ts... params) {
    std::forward_list<std::string> vs{params...};
    std::forward_list<mysqlshdk::utils::Version> vers;
    for (const auto &v : vs) vers.emplace_front(mysqlshdk::utils::Version(v));
    s_available_checks.emplace_back(std::move(vers), creator);
    return true;
  }

  static bool register_check(Creator creator,
                             const mysqlshdk::utils::Version &ver) {
    s_available_checks.emplace_back(
        std::forward_list<mysqlshdk::utils::Version>{ver}, creator);
    return true;
  }

  static void prepare_translation_file(const char *filename);

  static std::vector<std::unique_ptr<Upgrade_check>> create_checklist(
      const std::string &src_ver, const std::string &dst_ver);

  explicit Upgrade_check(const char *name) : m_name(name) {}
  virtual ~Upgrade_check() {}

  virtual const char *get_name() const { return m_name; }
  virtual const char *get_title() const;
  virtual const char *get_description() const;
  virtual const char *get_doc_link() const;

  virtual std::vector<Upgrade_issue> run(
      std::shared_ptr<mysqlshdk::db::ISession> session) = 0;

 protected:
  virtual const char *get_description_internal() const { return nullptr; }
  virtual const char *get_doc_link_internal() const { return nullptr; }
  virtual const char *get_title_internal() const { return nullptr; }

  const char *m_name;

 private:
  static const char *get_text(const char *name, const char *field);
  static Collection s_available_checks;
};

class Sql_upgrade_check : public Upgrade_check {
 public:
  static std::unique_ptr<Sql_upgrade_check> get_reserved_keywords_check();
  static std::unique_ptr<Sql_upgrade_check> get_utf8mb3_check();
  static std::unique_ptr<Sql_upgrade_check> get_innodb_rowformat_check();
  static std::unique_ptr<Sql_upgrade_check> get_zerofill_check();
  static std::unique_ptr<Sql_upgrade_check> get_mysql_schema_check();
  static std::unique_ptr<Sql_upgrade_check> get_old_temporal_check();
  static std::unique_ptr<Sql_upgrade_check> get_foreign_key_length_check();
  static std::unique_ptr<Sql_upgrade_check> get_maxdb_sql_mode_flags_check();
  static std::unique_ptr<Sql_upgrade_check> get_obsolete_sql_mode_flags_check();
  static std::unique_ptr<Sql_upgrade_check> get_enum_set_element_length_check();
  static std::unique_ptr<Sql_upgrade_check>
  get_partitioned_tables_in_shared_tablespaces_check(
      const mysqlshdk::utils::Version &ver);
  static std::unique_ptr<Sql_upgrade_check> get_removed_functions_check();
  static std::unique_ptr<Sql_upgrade_check> get_groupby_asc_syntax_check();

  Sql_upgrade_check(const char *name, const char *title,
                    std::vector<std::string> &&queries,
                    Upgrade_issue::Level level = Upgrade_issue::WARNING,
                    const char *advice = "",
                    std::forward_list<std::string> &&set_up =
                        std::forward_list<std::string>(),
                    std::forward_list<std::string> &&clean_up =
                        std::forward_list<std::string>());

  std::vector<Upgrade_issue> run(
      std::shared_ptr<mysqlshdk::db::ISession> session) override;

  const char *get_description_internal() const override;
  const char *get_title_internal() const override;

 protected:
  virtual Upgrade_issue parse_row(const mysqlshdk::db::IRow *row);
  std::vector<std::string> m_queries;
  std::forward_list<std::string> m_set_up;
  std::forward_list<std::string> m_clean_up;
  const Upgrade_issue::Level m_level;
  std::string m_title;
  std::string m_advice;
  std::string m_doc_link;
};

class Check_table_command : public Upgrade_check {
 public:
  Check_table_command();

  std::vector<Upgrade_issue> run(
      std::shared_ptr<mysqlshdk::db::ISession> session) override;

 protected:
  const char *get_description_internal() const override { return nullptr; }

  const char *get_title_internal() const override {
    return "Issues reported by 'check table x for upgrade' command";
  }
};

} /* namespace mysqlsh */

#endif  // MODULES_UTIL_UPGRADE_CHECK_H_
