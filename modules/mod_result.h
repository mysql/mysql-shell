/*
 * Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MOD_RESULT_H_
#define _MOD_RESULT_H_

#include "shellcore/types.h"
#include "shellcore/types_cpp.h"
#include "mod_mysqlx.h"
#include <list>

#undef ERROR //Needed to avoid conflict with ERROR in mysqlx.pb.h
#include "mysqlx.pb.h"

namespace mysh
{
  class X_resultset : public Base_resultset
  {
  public:
    X_resultset(boost::shared_ptr<X_connection> owner,
                bool has_data,
                int cursor_id,
                uint64_t affected_rows,
                uint64_t last_insert_id,
                int warning_count,
                const char *info,
                int next_mid,
                Message* next_message,
                bool expect_metadata,
                boost::shared_ptr<shcore::Value::Map_type> options = boost::shared_ptr<shcore::Value::Map_type>());
    virtual ~X_resultset();

    virtual std::string class_name() const  { return "X_resultset"; }
    virtual int fetch_metadata();

    void reset(unsigned long duration = -1, int next_mid = 0, ::google::protobuf::Message* next_message = NULL);
    void set_result_metadata(Message *msg);
    int get_cursor_id() { return _cursor_id; }
    bool is_all_fetch_done()  { return _all_fetch_done; }
    bool is_current_fetch_done()  { return _current_fetch_done; }
    void flush_messages(bool complete);

  protected:
    virtual Base_row* next_row();

    boost::weak_ptr<X_connection> _xowner;

  private:
    int _cursor_id;
    int _next_mid;
    Message* _next_message;

    bool _expect_metadata;
    bool _current_fetch_done;
    bool _all_fetch_done;
  };

  class Collection_resultset : public X_resultset
  {
  public:
    Collection_resultset(boost::shared_ptr<X_connection> owner,
    bool has_data,
    int cursor_id,
    uint64_t affected_rows,
    uint64_t last_insert_id,
    int warning_count,
    const char *info,
    int next_mid,
    Message* next_message,
    bool expect_metadata,
    boost::shared_ptr<shcore::Value::Map_type> options = boost::shared_ptr<shcore::Value::Map_type>());

    virtual std::string class_name() const  { return "CollectionResultset"; }
    virtual shcore::Value next(const shcore::Argument_list &args);
    virtual shcore::Value fetch_all(const shcore::Argument_list &args);
  };
};

#endif
