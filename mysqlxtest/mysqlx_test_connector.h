/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MYSQLX_CONNECTOR_H_
#define _MYSQLX_CONNECTOR_H_

#include <boost/asio.hpp>
#include <google/protobuf/message.h>

#undef ERROR //Needed to avoid conflict with ERROR in mysqlx.pb.h
#include "mysqlx.pb.h"

typedef google::protobuf::Message Message;

namespace mysqlx {
  bool parse_mysql_connstring(const std::string &connstring,
    std::string &protocol, std::string &user, std::string &password,
    std::string &host, int &port, std::string &sock,
    std::string &db, int &pwd_found);

  class Mysqlx_test_connector
  {
    typedef boost::asio::ip::tcp tcp;

  public:
    Mysqlx_test_connector(const std::string &host, const std::string &port);
    ~Mysqlx_test_connector();

    void send_message(int mid, Message *msg);
    void send_message(int mid, const std::string &mdata);
    Message *read_response(int &mid);
    Message *send_receive_message(int& mid, Message *msg, std::set<int> responses, const std::string& info);
    Message *send_receive_message(int& mid, Message *msg, int response_id, const std::string& info);
    void handle_wrong_response(int mid, Message *msg, const std::string& info);

    void flush();
    void close();

    // Overrides for Client Session Messages
    void send_message(Mysqlx::Session::AuthenticateStart *m){ send_message(Mysqlx::ClientMessages_Type_SESS_AUTH_START, m); };
    void send_message(Mysqlx::Session::AuthenticateContinue *m){ send_message(Mysqlx::ClientMessages_Type_SESS_AUTH_CONT, m); };
    void send_message(Mysqlx::Session::Reset *m){ send_message(Mysqlx::ClientMessages_Type_SESS_RESET, m); };
    void send_message(Mysqlx::Session::Close *m){ send_message(Mysqlx::ClientMessages_Type_SESS_CLOSE, m); };

    // Overrides for SQL Messages
    void send_message(Mysqlx::Sql::PrepareStmt *m){ send_message(Mysqlx::ClientMessages_Type_SQL_PREP_STMT_CLOSE, m); };
    void send_message(Mysqlx::Sql::PreparedStmtClose *m){ send_message(Mysqlx::ClientMessages_Type_SQL_PREP_STMT, m); };
    void send_message(Mysqlx::Sql::PreparedStmtExecute *m){ send_message(Mysqlx::ClientMessages_Type_SQL_PREP_STMT_EXEC, m); };
    void send_message(Mysqlx::Sql::CursorFetchMetaData *m){ send_message(Mysqlx::ClientMessages_Type_SQL_CURSOR_FETCH_META, m); };
    void send_message(Mysqlx::Sql::CursorFetchRows *m){ send_message(Mysqlx::ClientMessages_Type_SQL_CURSOR_FETCH_ROWS, m); };
    void send_message(Mysqlx::Sql::CursorsPoll *m){ send_message(Mysqlx::ClientMessages_Type_SQL_CURSORS_POLL, m); };
    void send_message(Mysqlx::Sql::CursorClose *m){ send_message(Mysqlx::ClientMessages_Type_SQL_CURSOR_CLOSE, m); };

    // Overrides for CRUD operations
    void send_message(Mysqlx::Crud::PrepareFind *m){ send_message(Mysqlx::ClientMessages_Type_CRUD_PREP_FIND, m); };
    void send_message(Mysqlx::Crud::PrepareInsert *m){ send_message(Mysqlx::ClientMessages_Type_CRUD_PREP_INSERT, m); };
    void send_message(Mysqlx::Crud::PrepareUpdate *m){ send_message(Mysqlx::ClientMessages_Type_CRUD_PREP_UPDATE, m); };
    void send_message(Mysqlx::Crud::PrepareDelete *m){ send_message(Mysqlx::ClientMessages_Type_CRUD_PREP_DELETE, m); };

    // Overrides for Connection
    void send_message(Mysqlx::Connection::CapabilitiesGet *m){ send_message(Mysqlx::ClientMessages_Type_CON_GET_CAP, m); };
    void send_message(Mysqlx::Connection::CapabilitiesSet *m){ send_message(Mysqlx::ClientMessages_Type_CON_SET_CAP, m); };
    void send_message(Mysqlx::Connection::Close *m){ send_message(Mysqlx::ClientMessages_Type_CON_CLOSE, m); };

  protected:
    boost::asio::io_service ios;
    boost::asio::io_service _ios;
    boost::asio::ip::tcp::socket _socket;
  };
};

#endif
