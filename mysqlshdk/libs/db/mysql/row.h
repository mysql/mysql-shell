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

#ifndef _CORELIBS_DB_MYSQL_ROW_H_
#define _CORELIBS_DB_MYSQL_ROW_H_

#include "mysqlshdk/libs/db/row.h"
#include "mysqlshdk/libs/db/column.h"
#include <vector>
#include <map>
#include <set>

#include <mysql.h>
#include <memory>

namespace mysqlshdk {
namespace db {
namespace mysql {

class SHCORE_PUBLIC Row : public mysqlshdk::db::IRow{
public:
  Row(MYSQL_ROW row, unsigned long *lengths,
      const std::vector<Column> &metadata);
  virtual ~Row() {}

  virtual size_t size() const;

  virtual int64_t get_int(int index) const;
  virtual uint64_t get_uint(int index) const;
  virtual std::string get_string(int index) const;
  virtual std::pair<const char*, size_t> get_data(int index) const;
  virtual double get_double(int index) const;
  virtual std::string get_date(int index) const;


  virtual bool is_null(int index) const;
  virtual bool is_int(int index) const;
  virtual bool is_uint(int index) const;
  virtual bool is_string(int index) const;
  virtual bool is_double(int index) const;
  virtual bool is_date(int index) const;
  virtual bool is_binary(int index) const;

private:
  MYSQL_ROW _row;
  std::vector<unsigned long> _lengths;
  const std::vector<Column> &_metadata;

  static std::map<std::string, std::set<Type> > _type_mappings;
  static std::string resolve_mysql_data_type(Type type);
  void validate_index(int index) const;
  bool validate_type(int index, const std::string& type, bool throw_on_error = false) const;
};

}
}
}
#endif
