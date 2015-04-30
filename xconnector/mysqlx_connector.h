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

namespace mysh {

  class Mysqlx_connector
  {
    typedef boost::asio::ip::tcp tcp;

    public:
      Mysqlx_connector(const std::string &host, const std::string &port);
      ~Mysqlx_connector();

      void send_message(int mid, Message *msg);
      void send_message(int mid, const std::string &mdata);
      Message *read_response(int &mid);
      Message *send_receive_message(int& mid, Message *msg, std::set<int> responses, const std::string& info);
      Message *send_receive_message(int& mid, Message *msg, int response_id, const std::string& info);
      void handle_wrong_response(int mid, Message *msg, const std::string& info);

      void flush();
      void close();

    protected:
      boost::asio::io_service ios;
      boost::asio::io_service _ios;
      boost::asio::ip::tcp::socket _socket;
      //boost::asio::buffered_write_stream<boost::asio::ip::tcp::socket> m_wstream;
  };
};

#endif
