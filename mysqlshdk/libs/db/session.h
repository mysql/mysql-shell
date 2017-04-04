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

#ifndef _CORELIBS_DB_ISESSION_H_
#define _CORELIBS_DB_ISESSION_H_

#include "mysqlshdk_export.h"
#include "mysqlshdk/libs/db/result.h"
#include "mysqlshdk/libs/db/ssl_info.h"

#include <fstream>
#include <iostream>

namespace mysqlshdk {
namespace db {
class SHCORE_PUBLIC ISession {
public:
  // Connection
  virtual void connect(const std::string& uri, const char *password = NULL) = 0;
  virtual void connect(const std::string &host, int port, const std::string &socket,
                        const std::string &user, const std::string &password, const std::string &schema,
                        const mysqlshdk::utils::Ssl_info& ssl_info) = 0;

  // Execution
  std::unique_ptr<IResult> query(const std::string& sql) { return query(sql, false); }
  virtual std::unique_ptr<IResult> query(const std::string& sql, bool buffered) = 0;
  virtual void execute(const std::string& sql) = 0;
  virtual void start_transaction() = 0;
  virtual void commit() = 0;
  virtual void rollback() = 0;
  virtual const char* get_ssl_cipher() = 0;

  // Disconnection
  virtual void close() = 0;

  virtual ~ISession() {}
};
}
}
#endif
