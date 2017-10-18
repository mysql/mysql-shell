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

// MySQL DB access module, for use by plugins and others
// For the module that implements interactive DB functionality see mod_db

#ifndef MYSQLSHDK_LIBS_DB_SESSION_RECORDER_H_
#define MYSQLSHDK_LIBS_DB_SESSION_RECORDER_H_

#include <memory>
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/session.h"

namespace mysqlshdk {
namespace db {
/**
 * This singleton handles the file stream used to log Mock activity
 * Using a singleton enables Mock recording even on unrelated classes
 */
class SHCORE_PUBLIC Mock_record {
 private:
  Mock_record() {
    const char* outfile = std::getenv("MOCK_RECORDING_FILE");
    if (outfile) {
      std::cerr << "Enabled mock recording at: " << outfile << std::endl;
      _file.open(outfile, std::ofstream::trunc | std::ofstream::out);
    }
  }
  std::ofstream _file;
  static Mock_record* _instance;

 public:
  static std::ofstream& get() {
    if (!_instance)
      _instance = new Mock_record();

    return _instance->_file;
  };
};

/**
 * Records Session GMOCK expectations based on the effective calls
 * as well as the right instructions to create Fake results
 * i.e. for Unit Tests
 */
class SHCORE_PUBLIC Session_recorder : public ISession {
 public:
  Session_recorder() : _target(nullptr) {}
  Session_recorder(ISession* target);
  virtual void connect(
      const mysqlshdk::db::Connection_options& connection_options);
  virtual std::shared_ptr<IResult> query(const std::string& sql, bool buffered);
  virtual void execute(const std::string& sql);
  virtual void start_transaction();
  virtual void commit();
  virtual void rollback();
  virtual void close();
  virtual const char* get_ssl_cipher() const;
  virtual bool is_open() const { return _target->is_open(); }

  void set_target(ISession* target) { _target = target; }

 private:
  ISession* _target;
};

/**
 * Records Result GMOCK expectations based on the effective calls
 * as well as the right instructions to create Fake results
 * i.e. for Unit Tests
 */
class SHCORE_PUBLIC Result_recorder : public IResult {
 public:
  Result_recorder(IResult* target);

  virtual const IRow* fetch_one();
  virtual bool next_resultset();
  virtual std::unique_ptr<Warning> fetch_one_warning();

  // Metadata retrieval
  virtual int64_t get_auto_increment_value() const;
  virtual bool has_resultset();

  virtual uint64_t get_affected_row_count() const;
  virtual uint64_t get_fetched_row_count() const;
  virtual uint64_t get_warning_count() const;
  virtual std::string get_info() const;

  virtual const std::vector<Column>& get_metadata() const;

 private:
  void save_result();
  void save_current_result(const std::vector<Column>& columns,
                           const std::vector<std::unique_ptr<IRow> >& rows);
  IResult* _target;

  std::vector<std::vector<Column>*> _all_metadata;
  std::vector<std::vector<std::unique_ptr<IRow> > > _all_results;
  std::vector<std::vector<std::unique_ptr<Warning> > > _all_warnings;
  size_t _rset_index;
  size_t _row_index;
  size_t _warning_index;

  std::string map_column_type(Type type);

  std::vector<Column> _empty_metadata;
};
}  // namespace db
}  // namespace mysqlshdk
#endif  // MYSQLSHDK_LIBS_DB_SESSION_RECORDER_H_
