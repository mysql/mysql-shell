/*
 * Copyright (c) 2022, 2024, Oracle and/or its affiliates.
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

#include "modules/adminapi/common/undo.h"
#include <memory>
#include <string>
#include <utility>

namespace mysqlsh {
namespace dba {

namespace {
class Function_undo : public Undo_tracker::Undo_entry {
 public:
  explicit Function_undo(std::function<void()> f) : m_fn(std::move(f)) {}

  void cancel() override { m_fn = {}; }
  bool call() override {
    if (m_fn) {
      std::function<void()> fn;
      std::swap(fn, m_fn);
      fn();
      return true;
    }
    return false;
  }

 private:
  std::function<void()> m_fn;
};

class Sql_undo : public Undo_tracker::Undo_entry {
 public:
  Sql_undo(mysqlshdk::mysql::Sql_undo_list u,
           const std::function<std::shared_ptr<Instance>()> &get_instance)
      : m_undo(std::move(u)), m_get_instance(get_instance) {}

  void cancel() override { m_cancel = true; }
  bool call() override {
    if (!m_cancel) {
      m_cancel = true;
      m_undo.execute(*m_get_instance());
      return true;
    }
    return false;
  }

 private:
  mysqlshdk::mysql::Sql_undo_list m_undo;
  std::function<std::shared_ptr<Instance>()> m_get_instance;
  bool m_cancel = false;
};
}  // namespace

Undo_tracker::Undo_entry &Undo_tracker::add(
    const std::string &note, mysqlshdk::mysql::Sql_undo_list sql_undo,
    const std::function<std::shared_ptr<Instance>()> &get_instance) {
  m_entries.emplace_front(
      note, std::make_unique<Sql_undo>(std::move(sql_undo), get_instance));

  return *m_entries.front().second;
}

Undo_tracker::Undo_entry &Undo_tracker::add_back(
    const std::string &note, mysqlshdk::mysql::Sql_undo_list sql_undo,
    const std::function<std::shared_ptr<Instance>()> &get_instance) {
  m_entries.emplace_back(
      note, std::make_unique<Sql_undo>(std::move(sql_undo), get_instance));

  return *m_entries.back().second;
}

Undo_tracker::Undo_entry &Undo_tracker::add(const std::string &note,
                                            const std::function<void()> &f) {
  m_entries.emplace_front(note, std::make_unique<Function_undo>(f));

  return *m_entries.front().second;
}

Undo_tracker::Undo_entry &Undo_tracker::add_back(
    const std::string &note, const std::function<void()> &f) {
  m_entries.emplace_back(note, std::make_unique<Function_undo>(f));

  return *m_entries.back().second;
}

void Undo_tracker::execute() {
  for (auto i = m_entries.begin(); i != m_entries.end(); ++i) {
    if (!(*i).first.empty()) log_info("Revert: %s", (*i).first.c_str());
    if (!(*i).second->call())
      log_info("Revert: %s (was cancelled)", (*i).first.c_str());
  }

  m_entries.clear();
}

}  // namespace dba
}  // namespace mysqlsh
